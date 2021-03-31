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
#ifndef CHANNELER_PIPE_ENQUEUE_MESSAGE_H
#define CHANNELER_PIPE_ENQUEUE_MESSAGE_H

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
 * The enqueue_message filter puts messages into a channel's outgoing
 * message queue.
 *
 * This is simply so that we don't keep pushing messages through the filter
 * chain. Instead, the buffers should be deciding on which message or packet
 * to process any given time.
 */
template <
  typename channelT,
  typename next_filterT,
  typename next_eventT
>
struct enqueue_message_filter
{
  using input_event = message_out_event;
  using channel_set = ::channeler::channels<channelT>;

  inline enqueue_message_filter(next_filterT * next,
      channel_set & channels)
    : m_next{next}
    , m_channels{channels}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    auto in = event_as<input_event>("egress:enqueue_message", ev.get(), ET_MESSAGE_OUT);

    // Get the appropriate channel data
    auto ch = m_channels.get(in->channel);
    if (!ch) {
      // Uh-oh, error
      // TODO
      return {};
    }

    // Enqueue message
    ch->enqueue_egress_message(std::move(in->message));

    // Create output event
    auto out = std::make_unique<message_out_enqueued_event>(in->channel);
    return m_next->consume(std::move(out));
  }


  next_filterT *  m_next;
  channel_set &   m_channels;
};


} // namespace channeler::pipe

#endif // guard
