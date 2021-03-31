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
#ifndef CHANNELER_PIPE_INGRESS_CHANNEL_ASSIGN_H
#define CHANNELER_PIPE_INGRESS_CHANNEL_ASSIGN_H

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

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {


/**
 * Assigns a packet to a channel.
 *
 * This effectively maps packets to the internal channel data structure.
 *
 * TODO
 * https://gitlab.com/interpeer/channeler/-/issues/19
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

    // There are three cases we need to consider:
    // a) We have a full channel. In that case, we need to assign the packet
    //    to said channel.
    // b) We have a pending channel and early data. This can only happen to
    //    initiators. Since the responder then clearly accepted our channel,
    //    we could pass the channel structure. But later filters should
    //    be able to distinguish this, so we pass an empty channel pointer.
    // c) We do not have either kind of channel. This we must reject.
    auto ptr = m_channel_set->get(in->packet.channel());
    if (!ptr) {
      return m_classifier.process(in->transport.source,
          in->transport.destination, in->packet);
    }

    // If we have an established channel here, we can place the packet in a
    // buffer. Otherwise, we clear the channel structure again to indicate
    // pending status.
    if (m_channel_set->has_established_channel(in->packet.channel())) {
      auto err = ptr->ingress_buffer_push(in->packet, in->data);
      if (ERR_SUCCESS != err) {
        // TODO: in future versions, we'll need to return flow control information
        //       to the sender.
        //       https://gitlab.com/interpeer/channeler/-/issues/2
        return {};
      }
    }
    else {
      ptr.reset();
    }

    // Need to construct a new event.
    auto next = std::make_unique<next_eventT>(
        in->transport.source,
        in->transport.destination,
        in->packet,
        in->data,
        ptr // TODO maybe we don't need to pass this on. Which could mean we
            //      may not need the next_eventT type.
    );
    return m_next->consume(std::move(next));
  }


  next_filterT *  m_next;
  channel_set *   m_channel_set;
  classifier      m_classifier; // TODO ptr or ref for shared state?
                                // https://gitlab.com/interpeer/channeler/-/issues/21
};


} // namespace channeler::pipe

#endif // guard
