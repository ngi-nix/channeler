/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2021 Jens Finkhaeuser.
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
#ifndef CHANNELER_PIPE_EVENT_AS_H
#define CHANNELER_PIPE_EVENT_AS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#if defined(DEBUG) && !defined(NDEBUG)
#include <iostream>
#endif // DEBUG

#include "event.h"

namespace channeler::pipe {

inline void
event_assert_set(char const * caller, event * raw)
{
#if defined(DEBUG) && !defined(NDEBUG)
  if (!raw) {
    std::cerr << caller << " received empty event." << std::endl;
    throw exception{ERR_INVALID_REFERENCE};
  }
  std::cout << caller << " received event type: " << raw->type << std::endl;
#endif // DEBUG
}

/**
 * Helper function for asserting that a given filter receives an event of a
 * given type.
 *
 * When DEBUG is defined, this is active code. When NDEBUG is defined, only
 * the conversion is performed.
 */
template <
  typename eventT
>
inline eventT *
event_as(char const * caller, event * raw, event_type expected_type)
{
  event_assert_set(caller, raw);

#if defined(DEBUG) && !defined(NDEBUG)
  if (raw->type != expected_type) {
    std::cerr << caller << " received unexpected event type: " << raw->type
      << " (wanted: " << expected_type << ")"
      << std::endl;
    throw exception{ERR_INVALID_PIPE_EVENT};
  }
#endif // DEBUG

  return reinterpret_cast<eventT *>(raw);
}


} // namespace channeler::pipe

#endif // guard
