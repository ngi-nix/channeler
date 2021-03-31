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
#ifndef CHANNELER_PIPE_OUT_BUFFER_H
#define CHANNELER_PIPE_OUT_BUFFER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>

#include "../event.h"
#include "../action.h"


namespace channeler::pipe {

/**
 * The out_buffer filter places a fully formed packet into an output buffer
 * for a channel.
 *
 * Note: we currently do this at the last instance, but best practice would
 *       require each resend attempt to be encrypted again to avoid providing
 *       an oracle.
 *
 *       We'll leave this out for now. When encryption is added, we can
 *       pick that issue up again. TODO
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT,
  typename next_filterT,
  typename next_eventT
>
struct out_buffer_filter
{
  using input_event = packet_out_event<POOL_BLOCK_SIZE>;
  using channel_set = ::channeler::channels<channelT>;
  using channel_ptr = typename channel_set::channel_ptr;

  inline out_buffer_filter(
      next_filterT * next,
      channel_set & channels
    )
    : m_next{next}
    , m_channels{channels}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_PACKET_OUT) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event const * in = reinterpret_cast<input_event const *>(ev.get());

    auto ptr = m_channels.get(in->packet.channel());
    if (!ptr) {
      // Uh-oh, error.
      // FIXME
      return {};
    }

    // The packet is finished, so it'll have to go into the egress buffer.
    auto err = ptr->egress_buffer_push(in->packet, in->slot);
    if (ERR_SUCCESS != err) {
      // Uh-oh, error. - this is a buffer overflow, must be reported to
      // the user
      return {};
    }

    // The next filter just gets a notification that the egress packet buffer
    // has data - we don't want to send a packet or slot, because it's up to
    // the buffer class to provide the output order.
    auto out = std::make_unique<packet_out_enqueued_event<channelT>>(ptr);
    return m_next->consume(std::move(out));
  }


  next_filterT *  m_next;
  channel_set &   m_channels;
};


} // namespace channeler::pipe

#endif // guard
