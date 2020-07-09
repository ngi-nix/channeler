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
#ifndef CHANNELER_VISIBILITY_H
#define CHANNELER_VISIBILITY_H

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
  #ifdef CHANNELER_IS_BUILDING
    #define CHANNELER_API __declspec(dllexport)
  #else
    #define CHANNELER_API __declspec(dllimport)
  #endif
  #define CHANNELER_API_FRIEND CHANNELER_API
#else // Windows
  #if __GNUC__ >= 4
    #define CHANNELER_API  [[gnu::visibility("default")]]
  #else
    #define CHANNELER_API
  #endif // GNU C
  #define CHANNELER_API_FRIEND
#endif // POSIX

// Private symbols may be exported in debug builds for testing purposes.
#if defined(DEBUG)
  #define CHANNELER_PRIVATE CHANNELER_API
#else
  #define CHANNELER_PRIVATE
#endif

#endif // guard
