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
#ifndef CHANNELER_CHANNEL_DATA_H
#define CHANNELER_CHANNEL_DATA_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <channeler/channelid.h>

namespace channeler {

/**
 * This data structure holds internal channel information, such as the
 * channel's buffers.
 *
 * TODO this is expected to chane a lot.
 */
struct channel_data
{
  inline channel_data(channelid const & id)
    : m_id{id}
  {
  }

  channelid m_id;
};

} // namespace channeler

#endif // guard
