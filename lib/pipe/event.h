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
#ifndef CHANNELER_PIPE_EVENT_H
#define CHANNELER_PIPE_EVENT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

namespace channeler::pipe {

/**
 * Events are pushed down the filter pipe. They have a type, and may carry
 * a type-dependent payload.
 */
enum event_type : uint_fast16_t
{
  ET_UNKNOWN = 0,
  ET_RAW_BUFFER,
  ET_PARSED_HEADER,
  ET_DECRYPTED_PACKET,
};


/**
 * Event base class
 */
struct event
{
  event_type const type = ET_UNKNOWN;
};


} // namespace channeler::pipe

#endif // guard
