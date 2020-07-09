/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright holder for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/
#ifndef CHANNELER_MACROS_H
#define CHANNELER_MACROS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <build-config.h>

#include <sstream>
#include <iomanip>

/**
 * Stringify the symbol passed to CHANNELER_SPRINGIFY()
 **/
#define CHANNELER_STRINGIFY(n) CHANNELER_STRINGIFY_HELPER(n)
#define CHANNELER_STRINGIFY_HELPER(n) #n

/**
 * Define logging symbols if -DDEBUG is specified (or -DNDEBUG isn't).
 *
 * Note: We can't use strerror_r() here because g++ turns on _GNU_SOURCE,
 *       which results in us getting the GNU version of the function rather
 *       than the POSIX version.
 **/
#if defined(DEBUG) && !defined(NDEBUG)
#include <iostream>
#include <string.h>
#define _LOG_BASE(severity, msg) { \
  std::stringstream sstream; \
  sstream << "[" << __FILE__ << ":" \
  << __LINE__ << "] " << severity << ": " << msg << std::endl; \
  std::cerr << sstream.str(); \
}
#define DLOG(msg) _LOG_BASE("DEBUG", msg)
#define ELOG(msg) _LOG_BASE("ERROR", msg)
#else // DEBUG
#define DLOG(msg)
#define ELOG(msg)
#endif // DEBUG

// Specific log macros - error codes and system errors
#if defined(CHANNELER_WIN32)
// FIXME #include <packeteer/util/string.h>
// FIXME #include <packeteer/net/netincludes.h>

#define ERR_LOG(msg, code) do { \
    TCHAR * errmsg = NULL; \
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, \
        NULL, code, \
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
        (LPWSTR) &errmsg, 0, NULL); \
    ELOG(msg \
      << " // [0x" << std::hex << code << std::dec << " (" << code << ")] " \
      << ::packeteer::util::to_utf8(errmsg)); \
    LocalFree(errmsg); \
  } while (false);
#define ERRNO_LOG(msg) ERR_LOG(msg, WSAGetLastError())
#else // CHANNELER_WIN32
#define ERR_LOG(msg, code) ELOG(msg << " // " << ::strerror(code))
#define ERRNO_LOG(msg) ERR_LOG(msg, errno)
#endif // CHANNELER_WIN32

// Exception logging
#define EXC_LOG(msg, exc) ELOG(msg << " // " << exc.what())

// error_t logging
#define ET_LOG(msg, code) ELOG(msg << " // " << error_name(code) << ": " \
    << error_message(code));

/**
 * Flow control guard; if this line is reached, an exception is thrown.
 **/
#define CHANNELER_FLOW_CONTROL_GUARD_WITH(msg)                      \
  {                                                                 \
    std::stringstream flowcontrol;                                  \
    std::string mess{msg};                                          \
    if (!mess.empty()) {                                            \
      flowcontrol << mess << " - ";                                 \
    }                                                               \
    flowcontrol << "Control should never have reached this line: "  \
      << __FILE__ << ":" << __LINE__;                               \
    throw exception(ERR_UNEXPECTED, flowcontrol.str());             \
  }

#define CHANNELER_FLOW_CONTROL_GUARD CHANNELER_FLOW_CONTROL_GUARD_WITH("")


/**
 * Support for clang/OCLint suppressions
 **/
#if defined(__clang__) and defined(OCLINT_IS_RUNNING)
#  define OCLINT_SUPPRESS(suppression) \
    __attribute__(( \
      annotate("oclint:suppress[" suppression "]") \
    ))
#else
#  define OCLINT_SUPPRESS(annotation)
#endif

#endif // guard
