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

#include "memory/packet_buffer.h"

namespace channeler {

/**
 * This data structure holds internal channel information, such as the
 * channel's buffers.
 *
 * TODO this is expected to chane a lot.
 */
template <
  std::size_t POOL_BLOCK_SIZE,
  typename lock_policyT = null_lock_policy
>
struct channel_data
{
  using buffer_type = memory::packet_buffer<POOL_BLOCK_SIZE, lock_policyT>;
  using slot_type = typename buffer_type::pool_type::slot;

  inline channel_data(channelid const & id, std::size_t packet_size,
      lock_policyT * lock = nullptr)
    : m_id{id}
    , m_lock{lock}
    , m_buffer{packet_size, m_lock}
  {
  }


  inline error_t buffer_push(packet_wrapper const & packet, slot_type slot)
  {
    return m_buffer.push(packet, slot);
  }

  // TODO pop for reading

  channelid       m_id;
  lock_policyT *  m_lock;
  buffer_type     m_buffer;
};

} // namespace channeler

#endif // guard
