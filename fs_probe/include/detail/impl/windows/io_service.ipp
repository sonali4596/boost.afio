/* io_service.hpp
Multiplex file i/o
(C) 2015 Niall Douglas http://www.nedprod.com/
File Created: Dec 2015


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "../../../io_service.hpp"
#include "import.hpp"

BOOST_AFIO_V2_NAMESPACE_BEGIN

io_service::io_service() : _work_queued(0)
{
  if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &_threadh, 0, false, DUPLICATE_SAME_ACCESS))
    throw std::runtime_error("Failed to create creating thread handle");
  _threadid = GetCurrentThreadId();
}

io_service::~io_service()
{
  if (_work_queued)
  {
    std::cerr << "WARNING: ~io_service() sees work still queued, blocking until no work queued" << std::endl;
    while (_work_queued)
      std::this_thread::yield();
  }
  CloseHandle(_threadh);
}

result<bool> io_service::run_until(deadline d) noexcept
{
  if (!_work_queued)
    return false;
  if (GetCurrentThreadId() != _threadid)
    return make_errored_result<bool>(EOPNOTSUPP);
  stl11::chrono::steady_clock::time_point began_steady;
  stl11::chrono::system_clock::time_point end_utc;
  if (d)
  {
    if (d.steady)
      began_steady = stl11::chrono::steady_clock::now();
    else
      end_utc = d.to_time_point();
  }
  DWORD tosleep = INFINITE;
  if (d)
  {
    stl11::chrono::milliseconds ms;
    if (d.steady)
      ms = stl11::chrono::duration_cast<stl11::chrono::milliseconds>((began_steady + stl11::chrono::nanoseconds(d.nsecs)) - stl11::chrono::steady_clock::now());
    else
      ms = stl11::chrono::duration_cast<stl11::chrono::milliseconds>(end_utc - stl11::chrono::system_clock::now());
    if (ms.count() < 0)
      tosleep = 0;
    else
      tosleep = (DWORD) ms.count();
  }
  // Execute any APCs queued to this thread
  if (!SleepEx(tosleep, true))
  {
    // Really a timeout?
    if (d)
    {
      if (d.steady)
      {
        if(stl11::chrono::steady_clock::now()>=(began_steady + stl11::chrono::nanoseconds(d.nsecs)))
          return make_errored_result<bool>(ETIMEDOUT);
      }
      else
      {
        if(stl11::chrono::system_clock::now()>=end_utc)
          return make_errored_result<bool>(ETIMEDOUT);
      }
    }
  }
  return _work_queued != 0;
}

void io_service::post(detail::function_ptr<void(io_service *)> &&f)
{
  void *data = nullptr;
  {
    post_info pi(this, std::move(f));
    std::lock_guard<decltype(_posts_lock)> g(_posts_lock);
    _posts.push_back(std::move(pi));
    data = (void *)&_posts.back();
  }
  PAPCFUNC apcf = [](ULONG_PTR data) {
    post_info *pi = (post_info *)data;
    pi->f(pi->service);
    pi->service->_post_done(pi);
  };
  if (QueueUserAPC(apcf, _threadh, (ULONG_PTR)data))
    _work_enqueued();
  else
  {
    post_info *pi = (post_info *)data;
    pi->service->_post_done(pi);
  }
}

BOOST_AFIO_V2_NAMESPACE_END
