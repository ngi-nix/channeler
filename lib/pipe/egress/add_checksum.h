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
#ifndef CHANNELER_PIPE_ADD_CHECKSUM_H
#define CHANNELER_PIPE_ADD_CHECKSUM_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>

#include "../../memory/packet_pool.h"
#include "../event.h"
#include "../action.h"
#include "../event_as.h"

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {

/**
 * The add_checksum filter adds the checksum to a packet.
 *
 * It's separate from the message bundling filter if only because checksum
 * addition may be part of an encryption filter later.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT
>
struct add_checksum_filter
{
  using input_event = packet_out_event<POOL_BLOCK_SIZE>;
  using pool_type = ::channeler::memory::packet_pool<
    POOL_BLOCK_SIZE
    // FIXME lock policy
  >;
  using slot_type = typename pool_type::slot;

  inline add_checksum_filter(next_filterT * next)
    : m_next{next}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    auto in = event_as<input_event>("egress:add_checksum", ev.get(), ET_PACKET_OUT);

    // Set checksum
    auto err = in->packet.update_checksum();
    if (ERR_SUCCESS != err) {
      // Uh-oh, some kind of error.
      // TODO add an action
      return {};
    }

    return m_next->consume(std::move(ev));
  }


  next_filterT *  m_next;
};


} // namespace channeler::pipe

#endif // guard
