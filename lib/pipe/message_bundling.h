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
#ifndef CHANNELER_PIPE_MESSAGE_BUNDLING_H
#define CHANNELER_PIPE_MESSAGE_BUNDLING_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <iostream> // FIXME

#include "../memory/packet_pool.h"
#include "event.h"
#include "action.h"

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {

/**
 * The message_bundling filter packs messages into a packet.
 *
 * TODO: For the time being, every message gets its own packet. A future
 *       revision of the filter should take into account:
 *       a) a time slot mechanism, whereby incomplete packets are sent when
 *          a timeout expires, but otherwise are held back for more messages
 *          to accumulate
 *       b) priority flags that can override this and either:
 *          - send a message on its own
 *          - flush the packet that contains the prioritized message
 *            immediately.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT,
  typename channelT
>
struct message_bundling_filter
{
  using input_event = message_out_enqueued_event;
  using pool_type = ::channeler::memory::packet_pool<
    POOL_BLOCK_SIZE
    // FIXME lock policy
  >;
  using slot_type = typename pool_type::slot;
  using channel_set = ::channeler::channels<channelT>;
  using peerid_function = std::function<peerid()>;

  inline message_bundling_filter(next_filterT * next,
      channel_set & channels,
      pool_type & pool,
      peerid_function own_peerid_func,
      peerid_function peer_peerid_func
    )
    : m_next{next}
    , m_channels{channels}
    , m_pool{pool}
    , m_own_peerid_func{own_peerid_func}
    , m_peer_peerid_func{peer_peerid_func}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_MESSAGE_OUT_ENQUEUED) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event const * in = reinterpret_cast<input_event const *>(ev.get());

    // Let's be paranoid and check that there is egress data.
    auto ch = m_channels.get(in->channel);
    if (!ch->has_egress_data_pending()) {
      // TODO what to do here?
      return {};
    }

    // Allocate memory. This is for creating the packet header, which is useful
    // for understanding the maximum payload size.
    slot_type slot = m_pool.allocate();
    ::channeler::packet_wrapper packet{slot.data(), slot.size(), false};

    // Populate the packet's metadata as far as we understand it.
    packet.packet_size() = slot.size(); // Fixed packet size helps hide
                                        // payloads
    packet.sender() = m_own_peerid_func();
    packet.recipient() = m_peer_peerid_func();
    packet.channel() = in->channel;
    // TODO flags

    // Grab the channel; we'll pack as many messages as fit into the packet
    // here.

    // // TODO much of this could be moved into the packet class:
    // - initialize with meta as in the next few llines
    // - have add_message() functions that does the message adding
    //   piecemeal
    // - add padding only when requesting the buffer

    size_t remaining = packet.max_payload_size();
    std::byte * offset = packet.payload();

    do {
      std::size_t next_size = ch->next_egress_message_size();
      if (!next_size || next_size > remaining) {
        break;
      }

      auto used = serialize_message(offset, remaining,
          std::move(ch->dequeue_egress_message()));
      offset += used;
      remaining -= used;
    } while (remaining > 0);

    // Update packet payload size. The remaining buffer is part of the packet
    // size, but not of the payload size.
    packet.payload_size() = packet.max_payload_size() - remaining;

    // Fill the remaining bytes with padding. We use a variant of PKCS#7
    // https://tools.ietf.org/html/rfc5652#section-6.3
    // The main difference is that we do have a packet size encoded, so
    // we do not care about the length of the padding being below 256
    // Bytes.
    uint8_t pad_value = remaining % 0xff;
    for (std::size_t i = 0 ; i < remaining ; ++i) {
      offset[i] = static_cast<std::byte>(pad_value);
    }

    // Pass slot and packet on to next filter
    auto next = std::make_unique<next_eventT>(
        std::move(slot),
        std::move(packet)
    );
    return m_next->consume(std::move(next));
  }


  next_filterT *  m_next;
  channel_set &   m_channels;
  pool_type &     m_pool;
  peerid_function m_own_peerid_func;
  peerid_function m_peer_peerid_func;
};


} // namespace channeler::pipe

#endif // guard
