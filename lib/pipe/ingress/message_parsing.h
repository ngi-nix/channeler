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
#ifndef CHANNELER_PIPE_INGRESS_MESSAGE_PARSING_H
#define CHANNELER_PIPE_INGRESS_MESSAGE_PARSING_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <set>

#include "../../memory/packet_pool.h"
#include "../../channels.h"
#include "../event.h"
#include "../action.h"
#include "../filter_classifier.h"
#include "../event_as.h"

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {


/**
 * Parses messages in a packet. Follow-on filters will receive individual
 * messages.
 *
 * Expects the next_eventT constructor to take
 * - a message wrapper rerpresenting the parsed message
 * - an (optional) channel data structure pointer
 * - transport source address
 * - transport destination address
 * - packet_wrapper
 * - a pool slot
 *
 * such as e.g. enqueued_packet_event
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT,
  typename channelT,
  typename peer_failure_policyT = null_policy<peerid_wrapper>,
  typename transport_failure_policyT = null_policy<addressT>
>
struct message_parsing_filter
{
  using input_event = enqueued_packet_event<addressT, POOL_BLOCK_SIZE, channelT>;
  using channel_set = ::channeler::channels<channelT>;
  using classifier = filter_classifier<addressT, peer_failure_policyT, transport_failure_policyT>;

  inline message_parsing_filter(next_filterT * next)
    : m_next{next}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    auto in = event_as<input_event>("ingress:message_parsing", ev.get(), ET_ENQUEUED_PACKET);

    // If there is no data passed, we should also throw.
    if (nullptr == in->data.data()) {
      throw exception{ERR_INVALID_REFERENCE};
    }

    // We have a packet and it belongs to a channel. Now we need to push
    // messages down the pipeline.
    // TODO if the packet channel is pending, then this packet should
    //      contain a MSG_PACKET_COOKIE. We need to process that message
    //      *first* in order to be able to process the others. This may
    //      require two iterations.
    //      https://gitlab.com/interpeer/channeler/-/issues/13
    action_list_type actions;
    for (auto msg : in->packet.get_messages()) {
      // Need to construct a new event per message
      auto next = std::make_unique<next_eventT>(
          in->transport.source,
          in->transport.destination,
          in->packet,
          in->data,
          in->channel,
          std::move(msg)
      );
      auto ret = m_next->consume(std::move(next));
      actions.merge(ret);
    }

    // The nice thing about the iterator interface is that if there were no
    // messages in the packet, or the packet has trailing junk, we don't have
    // to care. If in->data slot hasn't been copied and the slot goes out
    // of scope when this input event is abandoned at the end of this function,
    // we've freed the pool slot effectively.

    return actions;
  }


  next_filterT *  m_next;
};


} // namespace channeler::pipe

#endif // guard
