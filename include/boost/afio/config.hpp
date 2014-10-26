/*
* File:   config.hpp
* Author: Paul Kirth
*
* Created on June 18, 2013, 7:30 PM
*/

 //  Copyright (c) 2013 Paul Kirth
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


// most of this comes from Boost.Atomic 

#ifndef BOOST_AFIO_CONFIG_HPP
#define BOOST_AFIO_CONFIG_HPP

#if !defined(BOOST_AFIO_HEADERS_ONLY) && !defined(BOOST_ALL_DYN_LINK)
#define BOOST_AFIO_HEADERS_ONLY 1
#endif

// Get Mingw to assume we are on at least Windows 2000
#if __MSVCRT_VERSION__ < 0x601
#undef __MSVCRT_VERSION__
#define __MSVCRT_VERSION__ 0x601
#endif

// Fix up mingw weirdness
#if !defined(WIN32) && defined(_WIN32)
#define WIN32 1
#endif
// Boost ASIO needs this
#if !defined(_WIN32_WINNT) && defined(WIN32)
#define _WIN32_WINNT 0x0501
#endif
#if defined(WIN32) && _WIN32_WINNT<0x0501
#error _WIN32_WINNT must at least be set to Windows XP for Boost ASIO to compile
#endif


#include "boost/config.hpp"  // Works standalone or with Boost
#include "../../boost.bindlib/include/import.hpp"
#ifndef BOOST_AFIO_V1_STL11_IMPL
#define BOOST_AFIO_V1_STL11_IMPL std
#endif
#define BOOST_AFIO_V1 (boost), (afio), (BOOST_LOCAL_BIND_NAMESPACE_VERSION(v1, BOOST_AFIO_V1_STL11_IMPL), inline)
#define BOOST_AFIO_V1_NAMESPACE       BOOST_LOCAL_BIND_NAMESPACE      (BOOST_AFIO_V1)
#define BOOST_AFIO_V1_NAMESPACE_BEGIN BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1)
#define BOOST_AFIO_V1_NAMESPACE_END   BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1)

#define BOOST_STL11_ATOMIC_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline))
#define BOOST_STL11_ATOMIC_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline))
#define BOOST_STL11_CHRONO_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline), (chrono))
#define BOOST_STL11_CHRONO_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline), (chrono))
#define BOOST_STL1z_FILESYSTEM_MAP_NAMESPACE_BEGIN    BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline), (filesystem))
#define BOOST_STL1z_FILESYSTEM_MAP_NAMESPACE_END      BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline), (filesystem))
#define BOOST_STL11_MUTEX_MAP_NAMESPACE_BEGIN         BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline))
#define BOOST_STL11_MUTEX_MAP_NAMESPACE_END           BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline))
#define BOOST_STL1z_NETWORKING_MAP_NAMESPACE_BEGIN    BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline), (asio))
#define BOOST_STL1z_NETWORKING_MAP_NAMESPACE_END      BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline), (asio))
#define BOOST_STL11_THREAD_MAP_NAMESPACE_BEGIN        BOOST_LOCAL_BIND_NAMESPACE_BEGIN(BOOST_AFIO_V1, (stl11, inline))
#define BOOST_STL11_THREAD_MAP_NAMESPACE_END          BOOST_LOCAL_BIND_NAMESPACE_END  (BOOST_AFIO_V1, (stl11, inline))
#include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_AFIO_V1_STL11_IMPL, atomic)
#include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_AFIO_V1_STL11_IMPL, chrono)
#include BOOST_LOCAL_BIND_INCLUDE_STL1z(BOOST_AFIO_V1_STL11_IMPL, filesystem)
#include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_AFIO_V1_STL11_IMPL, mutex)
#include BOOST_LOCAL_BIND_INCLUDE_STL1z(BOOST_AFIO_V1_STL11_IMPL, networking)
#include BOOST_LOCAL_BIND_INCLUDE_STL11(BOOST_AFIO_V1_STL11_IMPL, thread)

///////////////////////////////////////////////////////////////////////////////
//  Set up dll import/export options
#if (defined(BOOST_AFIO_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)) && \
    !defined(BOOST_AFIO_STATIC_LINK)

#if defined(BOOST_AFIO_SOURCE)
#undef BOOST_AFIO_HEADERS_ONLY
#define BOOST_AFIO_DECL BOOST_SYMBOL_EXPORT
#define BOOST_AFIO_BUILD_DLL
#else
#define BOOST_AFIO_DECL
#endif
#else
# define BOOST_AFIO_DECL
#endif // building a shared library


///////////////////////////////////////////////////////////////////////////////
//  Auto library naming
#if !defined(BOOST_AFIO_SOURCE) && !defined(BOOST_ALL_NO_LIB) && \
    !defined(BOOST_AFIO_NO_LIB)

#define BOOST_LIB_NAME boost_afio

// tell the auto-link code to select a dll when required:
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_AFIO_DYN_LINK)
#define BOOST_DYN_LINK
#endif

#include <boost/config/auto_link.hpp>

#endif  // auto-linking disabled

//#define BOOST_THREAD_VERSION 4
//#define BOOST_THREAD_PROVIDES_VARIADIC_THREAD
//#define BOOST_THREAD_DONT_PROVIDE_FUTURE
//#define BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK
#if BOOST_AFIO_HEADERS_ONLY == 1
# define BOOST_AFIO_HEADERS_ONLY_FUNC_SPEC inline
# define BOOST_AFIO_HEADERS_ONLY_MEMFUNC_SPEC inline
# define BOOST_AFIO_HEADERS_ONLY_VIRTUAL_SPEC inline virtual
// GCC gets upset if inline virtual functions aren't defined
# ifdef BOOST_GCC
#  define BOOST_AFIO_HEADERS_ONLY_VIRTUAL_UNDEFINED_SPEC { BOOST_AFIO_THROW_FATAL(std::runtime_error("Attempt to call pure virtual member function")); abort(); }
# else
#  define BOOST_AFIO_HEADERS_ONLY_VIRTUAL_UNDEFINED_SPEC =0;
# endif
#else
# define BOOST_AFIO_HEADERS_ONLY_FUNC_SPEC extern BOOST_AFIO_DECL
# define BOOST_AFIO_HEADERS_ONLY_MEMFUNC_SPEC
# define BOOST_AFIO_HEADERS_ONLY_VIRTUAL_SPEC virtual
# define BOOST_AFIO_HEADERS_ONLY_VIRTUAL_UNDEFINED_SPEC =0;
#endif

#if defined(__has_feature)
# if __has_feature(thread_sanitizer)
# define BOOST_AFIO_DISABLE_THREAD_SANITIZE __attribute__((no_sanitize_thread))
# endif
#endif
#ifndef BOOST_AFIO_DISABLE_THREAD_SANITIZE
# define BOOST_AFIO_DISABLE_THREAD_SANITIZE
#endif

#endif  /* BOOST_AFIO_CONFIG_HPP */

