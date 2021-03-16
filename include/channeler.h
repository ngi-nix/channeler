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
#ifndef CHANNELER_CHANNELER_H
#define CHANNELER_CHANNELER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

/**
 * Which platform are we on?
 */
#if !defined(CHANNELER_PLATFORM_DEFINED)
#  if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#    define CHANNELER_WIN32
#  else
#    define CHANNELER_POSIX
#  endif
#  define CHANNELER_PLATFORM_DEFINED
#endif

/**
 * Anonymous structs are a C++ extension for GNU
 */
#if defined(__GNUC__)
#  define CHANNELER_ANONYMOUS __extension__
#else
#  define CHANNELER_ANONYMOUS
#endif

/**
 * Decide what to include globally
 **/
#if defined(CHANNELER_WIN32)
// Include windows.h with minimal definitions
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define __UNDEF_LEAN_AND_MEAN
#  endif
#  define NOMINMAX
// Unicode builds
#  define UNICODE
#  define _UNICODE
#  include <windows.h>
#  include <WinDef.h>
#  ifdef __UNDEF_LEAN_AND_MEAN
#    undef WIN32_LEAN_AND_MEAN
#    undef __UNDEF_LEAN_AND_MEAN
#  endif
#endif

// Visibility macros are used by all, so they must come first.
#include <channeler/visibility.h>

// We want standard int types across the board.
#include <liberate/types.h>

// Not all, but the very basic headers are always included.
#include <channeler/error.h>
#include <channeler/version.h>

// TODO: include API headers if necessary

#endif // guard
