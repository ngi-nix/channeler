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
  typename next_eventT
>
struct message_bundling_filter
{
  using input_event = message_out_event;
  using pool_type = ::channeler::memory::packet_pool<
    POOL_BLOCK_SIZE
    // FIXME lock policy
  >;
  using slot_type = typename pool_type::slot;

  inline message_bundling_filter(next_filterT * next,
      pool_type & pool)
    : m_next{next}
    , m_pool{pool}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_MESSAGE_OUT) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event const * in = reinterpret_cast<input_event const *>(ev.get());

    // Allocate memory. This is for creating the packet header, which is useful
    // for understanding the maximum payload size.
    slot_type slot = m_pool.allocate();
    ::channeler::packet_wrapper packet{slot.data(), slot.size(), false};

    // Ensure that the message size does not exceed the packet size.
    // TODO we don't fragment mesages yet, but this may have to happen here.
    if (in->message->buffer_size > packet.max_payload_size()) {
      // FIXME create some kind of action to warn the user
      return {};
    }

    // TODO much of this could be moved into the packet class:
    // - initialize with meta as in the next few llines
    // - have add_message() functions that does the message adding
    //   piecemeal
    // - add padding only when requesting the buffer
    //
    // // TODO much of this could be moved into the packet class:
    // - initialize with meta as in the next few llines
    // - have add_message() functions that does the message adding
    //   piecemeal
    // - add padding only when requesting the buffer

    // Populate the packet's metadata as far as we understand it.
    packet.packet_size() = slot.size(); // Fixed packet size helps hide
                                        // payloads
    packet.sender() = in->sender;
    packet.recipient() = in->recipient;
    packet.channel() = in->channel;
    // TODO flags

    // Serialize message into the payload
    std::size_t payload_size = in->message->buffer_size;
    memcpy(packet.payload(), in->message->buffer, in->message->buffer_size);
    auto remain = packet.max_payload_size() - payload_size;

    // Fill the remaining bytes with padding. We use a variant of PKCS#7
    // https://tools.ietf.org/html/rfc5652#section-6.3
    // The main difference is that we do have a packet size encoded, so
    // we do not care about the length of the padding being below 256
    // Bytes.
    uint8_t pad_value = remain % 0xff;
    std::byte * offset = packet.payload() + payload_size;
    for (std::size_t i = 0 ; i < remain ; ++i) {
      offset[i] = static_cast<std::byte>(pad_value);
    }

    // Update packet payload size. The remaining buffer is part of the packet
    // size, but not of the payload size.
    packet.payload_size() = payload_size;

    // Pass slot and packet on to next filter
    auto next = std::make_unique<next_eventT>(
        std::move(slot),
        std::move(packet)
    );
    return m_next->consume(std::move(next));
  }


  next_filterT *  m_next;
  pool_type &     m_pool;
};


} // namespace channeler::pipe

#endif // guard
