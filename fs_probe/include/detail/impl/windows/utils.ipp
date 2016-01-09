/* utils.hpp
Misc utilities
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

#include "../../../utils.hpp"
#include "import.hpp"

BOOST_AFIO_V2_NAMESPACE_BEGIN

namespace utils
{
  // Stupid MSVC ...
  namespace detail { using namespace BOOST_AFIO_V2_NAMESPACE::detail; }
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 6387) // MSVC sanitiser warns that GetModuleHandleA() might fail (hah!)
#endif
  std::vector<size_t> page_sizes(bool only_actually_available) noexcept
  {
    static spinlock<bool> lock;
    static std::vector<size_t> pagesizes, pagesizes_available;
    stl11::lock_guard<decltype(lock)> g(lock);
    if (pagesizes.empty())
    {
      typedef size_t(WINAPI *GetLargePageMinimum_t)(void);
      SYSTEM_INFO si = { {0} };
      GetSystemInfo(&si);
      pagesizes.push_back(si.dwPageSize);
      pagesizes_available.push_back(si.dwPageSize);
      GetLargePageMinimum_t GetLargePageMinimum_ = (GetLargePageMinimum_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetLargePageMinimum");
      if (GetLargePageMinimum_)
      {
        windows_nt_kernel::init();
        using namespace windows_nt_kernel;
        pagesizes.push_back(GetLargePageMinimum_());
        /* Attempt to enable SeLockMemoryPrivilege */
        HANDLE token;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
        {
          auto untoken = detail::Undoer([&token] {CloseHandle(token); });
          TOKEN_PRIVILEGES privs = { 1 };
          if (LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &privs.Privileges[0].Luid))
          {
            privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if (AdjustTokenPrivileges(token, FALSE, &privs, 0, NULL, NULL) && GetLastError() == S_OK)
              pagesizes_available.push_back(GetLargePageMinimum_());
          }
        }
      }
    }
    return only_actually_available ? pagesizes_available : pagesizes;
  }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  void random_fill(char *buffer, size_t bytes)
  {
    windows_nt_kernel::init();
    using namespace windows_nt_kernel;
    if (!RtlGenRandom(buffer, (ULONG)bytes))
    {
      BOOST_AFIO_LOG_FATAL_EXIT("afio: Kernel crypto function failed");
      std::terminate();
    }
  }

  namespace detail
  {
    large_page_allocation allocate_large_pages(size_t bytes)
    {
      large_page_allocation ret(calculate_large_page_allocation(bytes));
      DWORD type = MEM_COMMIT | MEM_RESERVE;
      if (ret.page_size_used>65536)
        type |= MEM_LARGE_PAGES;
      ret.p = VirtualAlloc(nullptr, ret.actual_size, type, PAGE_READWRITE);
      if (!ret.p)
      {
        if (ERROR_NOT_ENOUGH_MEMORY == GetLastError())
          ret.p = VirtualAlloc(nullptr, ret.actual_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
      }
  #ifndef NDEBUG
      else if (ret.page_size_used>65536)
        std::cout << "afio: Large page allocation successful" << std::endl;
  #endif
      return ret;
    }
    void deallocate_large_pages(void *p, size_t bytes)
    {
      if (!VirtualFree(p, 0, MEM_RELEASE))
      {
        BOOST_AFIO_LOG_FATAL_EXIT("afio: Freeing large pages failed");
        std::terminate();
      }
    }
  }
}

BOOST_AFIO_V2_NAMESPACE_END