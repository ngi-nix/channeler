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
#ifndef CHANNELER_PIPE_CHANNEL_ASSIGN_H
#define CHANNELER_PIPE_CHANNEL_ASSIGN_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <set>

#include "../memory/packet_pool.h"
#include "../channels.h"
#include "event.h"
#include "action.h"
#include "filter_classifier.h"

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {


/**
 * Assigns a packet to a channel.
 *
 * This effectively maps packets to the internal channel data structure.
 *
 * TODO
 *
 * Expects the next_eventT constructor to take
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
struct channel_assign_filter
{
  using input_event = decrypted_packet_event<addressT, POOL_BLOCK_SIZE>;
  using channel_set = ::channeler::channels<channelT>;
  using classifier = filter_classifier<addressT, peer_failure_policyT, transport_failure_policyT>;

  inline channel_assign_filter(next_filterT * next, channel_set * chs,
      peer_failure_policyT * peer_p = nullptr,
      transport_failure_policyT * trans_p = nullptr)
    : m_next{next}
    , m_channel_set{chs}
    , m_classifier{peer_p, trans_p}
  {
    if (nullptr == m_channel_set) {
      throw exception{ERR_INVALID_REFERENCE};
    }
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_DECRYPTED_PACKET) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event * in = reinterpret_cast<input_event *>(ev.get());

    // If there is no data passed, we should also throw.
    if (nullptr == in->data.data()) {
      throw exception{ERR_INVALID_REFERENCE};
    }

    // Channels are *added* in the protocol handling filter. Here, we
    // short-circuit this for just the default channel.
    if (in->packet.channel() == DEFAULT_CHANNELID) {
      auto err = m_channel_set->add(in->packet.channel());
      if (ERR_SUCCESS != err) {
        // The packet is invalid, and we should drop it.
        return m_classifier.process(in->transport.source,
            in->transport.destination, in->packet);
      }
    }

    // If we can get a channel structure, it means the channel is established
    // and we can pass it on. Otherwise, we have to drop it. We cannot respond
    // to channels that have not been established yet.
    // XXX: Technically, we can easily just accept any channel identifier as
    //      correct - but channels get created by user actions. That means that
    //      any unknown channel is not the result of a user action, so could be
    //      sent maliciously to flood buffers, as the application is not going
    //      to be interested in draining the buffer. Best to drop the packet.
    auto ptr = m_channel_set->get(in->packet.channel());
    if (!ptr) {
      // If there is *no* channel strucutre, it may be that the channel is
      // pending and we're receiving early data. This is something we need
      // to pass on without a channel structure present.
      if (!m_channel_set->has_pending_channel(in->packet.channel())) {
        return m_classifier.process(in->transport.source,
            in->transport.destination, in->packet);
      }
    }

    // Need to construct a new event.
    auto next = std::make_unique<next_eventT>(
        in->transport.source,
        in->transport.destination,
        in->packet,
        in->data,
        ptr
    );
    return m_next->consume(std::move(next));
  }


  next_filterT *  m_next;
  channel_set *   m_channel_set;
  classifier      m_classifier; // TODO ptr or ref for shared state?
};


} // namespace channeler::pipe

#endif // guard
