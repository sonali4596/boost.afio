/* child_process.hpp
Routines for handling child processes
(C) 2016 Niall Douglas http://www.nedprod.com/
File Created: Marc 2016


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

#include "../../child_process.hpp"
#include "import.hpp"

BOOST_AFIO_V2_NAMESPACE_BEGIN

namespace detail
{
  child_process::~child_process()
  {
    wait();
    if(_processh)
    {
      CloseHandle(_processh.h);
      _processh.h = nullptr;
    }
    if(_readh)
    {
      CloseHandle(_readh.h);
      _readh.h = nullptr;
    }
    if(_writeh)
    {
      CloseHandle(_writeh.h);
      _writeh.h = nullptr;
    }
    if(_errh)
    {
      CloseHandle(_errh.h);
      _errh.h = nullptr;
    }
  }

  BOOST_AFIO_HEADERS_ONLY_MEMFUNC_SPEC result<child_process> child_process::launch(stl1z::filesystem::path __path, std::vector<stl1z::filesystem::path::string_type> __args, std::map<stl1z::filesystem::path::string_type, stl1z::filesystem::path::string_type> __env) noexcept
  {
    using string_type = stl1z::filesystem::path::string_type;
    using char_type = string_type::value_type;
    child_process ret(std::move(__path), std::move(__args), std::move(__env));

    STARTUPINFO si = {sizeof(STARTUPINFO)};
    si.dwFlags = STARTF_USESTDHANDLES;
    if(!CreatePipe(&si.hStdInput, &ret._writeh.h, nullptr, 0))
      return make_errored_result<child_process>(GetLastError());
    auto &unstdinput = Undoer([&si] { CloseHandle(si.hStdInput); });
    if(!CreatePipe(&ret._readh.h, &si.hStdOutput, nullptr, 0))
      return make_errored_result<child_process>(GetLastError());
    auto &unstdoutput = Undoer([&si] { CloseHandle(si.hStdOutput); });
    if(!CreatePipe(&ret._errh.h, &si.hStdError, nullptr, 0))
      return make_errored_result<child_process>(GetLastError());
    auto &unstderr = Undoer([&si] { CloseHandle(si.hStdError); });

    if(!SetHandleInformation(si.hStdInput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
      return make_errored_result<child_process>(GetLastError());
    if(!SetHandleInformation(si.hStdOutput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
      return make_errored_result<child_process>(GetLastError());
    if(!SetHandleInformation(si.hStdError, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT))
      return make_errored_result<child_process>(GetLastError());

    PROCESS_INFORMATION pi;
    char_type argsbuffer[32768], *argsbuffere = argsbuffer;
    for(auto &arg : ret._args)
    {
      if(argsbuffere - argsbuffer + arg.size() + 1 >= 32767)
        return make_errored_result<child_process>(EOVERFLOW);
      memcpy(argsbuffere, arg.data(), sizeof(char_type) * arg.size());
      argsbuffere += arg.size();
      *argsbuffere++ = ' ';
    }
    *argsbuffere = 0;
    char_type envbuffer[32768], *envbuffere = envbuffer;
    for(auto &env : ret._env)
    {
      if(envbuffere - envbuffer + env.first.size() + env.second.size() + 2 >= 32767)
        return make_errored_result<child_process>(EOVERFLOW);
      memcpy(envbuffere, env.first.data(), sizeof(char_type) * env.first.size());
      envbuffere += env.first.size();
      *envbuffere++ = '=';
      memcpy(envbuffere, env.first.data(), sizeof(char_type) * (1 + env.first.size()));
      envbuffere += env.first.size() + 1;
    }
    *envbuffere = 0;
    if(!CreateProcess(ret._path.c_str(), argsbuffer, nullptr, nullptr, true, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, envbuffer, nullptr, &si, &pi))
      return make_errored_result<child_process>(GetLastError());

    ret._processh.h = pi.hProcess;
    CloseHandle(pi.hThread);
    return ret;
  }

  bool child_process::is_running() const noexcept
  {
    DWORD retcode = 0;
    if(!GetExitCodeProcess(_processh.h, &retcode))
      return false;
    return retcode == STILL_ACTIVE;
  }

  result<intptr_t> child_process::wait_until(deadline d) noexcept
  {
    BOOST_AFIO_WIN_DEADLINE_TO_SLEEP_INIT(d);
    for(;;)
    {
      BOOST_AFIO_WIN_DEADLINE_TO_SLEEP_LOOP(d);
      HANDLE hs[2] = {_processh.h, sleep_object};
      DWORD ret = WaitForMultipleObjectsEx(sleep_object ? 2 : 1, hs, false, sleep_interval, true);
      switch(ret)
      {
      case WAIT_IO_COMPLETION:
        // loop
        break;
      case WAIT_OBJECT_0:
      {
        DWORD retcode = 0;
        if(!GetExitCodeProcess(_processh.h, &retcode))
          return make_errored_result<intptr_t>(GetLastError());
        return (intptr_t) retcode;
      }
      case WAIT_OBJECT_0 + 1:
      {
        // Really a timeout?
        BOOST_AFIO_WIN_DEADLINE_TO_TIMEOUT(intptr_t, d);
        break;
      }
      default:
        return make_errored_result<intptr_t>(GetLastError());
      }
    }
  }

  stl1z::filesystem::path current_process_path()
  {
    stl1z::filesystem::path::string_type buffer(32768, 0);
    DWORD len = GetModuleFileName(nullptr, const_cast<stl1z::filesystem::path::string_type::value_type *>(buffer.data()), buffer.size());
    if(!len)
      throw std::system_error(GetLastError(), std::system_category());
    buffer.resize(len);
    return stl1z::filesystem::path(std::move(buffer));
  }

  std::map<stl1z::filesystem::path::string_type, stl1z::filesystem::path::string_type> current_process_env()
  {
    using string_type = stl1z::filesystem::path::string_type;
    std::map<string_type, string_type> ret;
    string_type::value_type *strings = GetEnvironmentStrings();
    auto &unstrings = Undoer([strings] { FreeEnvironmentStrings(strings); });
    for(auto *s = strings, *e = strings; *s; s = (e = e + 1))
    {
      auto *c = s;
      for(c = nullptr; *e; e++)
      {
        if(!c && *e == '=')
          c = e;
      }
      ret.insert(string_type(s, c - s), string_type(c + 1, e - c - 1));
    }
    return ret;
  }
}

BOOST_AFIO_V2_NAMESPACE_END