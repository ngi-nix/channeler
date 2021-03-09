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
#ifndef CHANNELER_PIPE_EVENT_H
#define CHANNELER_PIPE_EVENT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <list>
#include <memory>

#include "../channels.h"
#include "../memory/packet_pool.h"

#include <channeler/packet.h>

namespace channeler::pipe {

/**
 * Events are pushed down the filter pipe. They have a type, and may carry
 * a type-dependent payload.
 */
enum event_type : uint_fast16_t
{
  ET_UNKNOWN = 0,
  ET_RAW_BUFFER,
  ET_PARSED_HEADER,
  // TODO encrypted or potentially encrypted packet might have a different
  // event type.
  ET_DECRYPTED_PACKET,
  ET_ENQUEUED_PACKET,
  ET_MESSAGE, // FIXME up til here incoming only; maybe we disambiguate at some point?

  ET_MESSAGE_OUT,
};


/**
 * Event base class
 */
struct event
{
  event_type const type;

  inline event(event_type t = ET_UNKNOWN)
    : type{t}
  {
  }

  virtual ~event() = default;
};

// Event list type
using event_list_type = std::list<std::shared_ptr<event>>;


// TODO pretty much all of the events carrying packet data can be merged into
//      one class, and have just the type change & asserted in each filter.
//      timeouts, etc. would need another class.


/**
 * The raw buffer event template just carries source and destination address
 * information from the transport level, in addition to the raw memory buffer.
 * The memory buffer is carried as an allocation slot from a memory pool, so
 * the memory pool's block size must also be known.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE
>
struct raw_buffer_event
  : public event
{
  // *** Types
  using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;
  using slot_type = typename pool_type::slot;

  // *** Data members
  struct {
    addressT source;
    addressT destination;
  } transport;

  slot_type data;

  // *** Constructor
  inline raw_buffer_event(addressT const & source,
      addressT const & destination,
      slot_type const & slot)
    : event{ET_RAW_BUFFER}
    , transport{source, destination}
    , data{slot}
  {
  }

  virtual ~raw_buffer_event() = default;
};

/**
 * The parsed header event template should cover most sitations where a filter
 * requires some header field information for their input.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE
>
struct parsed_header_event
  : public raw_buffer_event<addressT, POOL_BLOCK_SIZE>
{
  // *** Types
  using super_t = raw_buffer_event<addressT, POOL_BLOCK_SIZE>;

  // *** Data members
  ::channeler::public_header_fields header;

  // *** Constructor
  inline parsed_header_event(addressT const & source,
      addressT const & destination,
      ::channeler::public_header_fields const & hdr,
      typename super_t::slot_type const & slot)
    : super_t{source, destination, slot}
    , header{hdr}
  {
    // Explicitly override the type
    *const_cast<event_type *>(&(this->type)) = ET_PARSED_HEADER;
  }


  virtual ~parsed_header_event() = default;
};


/**
 * Similar to the parsed_header_event, the decrypted_packet_event contains
 * a little more information over the headers, namely the fully parsed and
 * decrypted packet.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE
>
struct decrypted_packet_event
  : public raw_buffer_event<addressT, POOL_BLOCK_SIZE>
{
  // *** Types
  using super_t = raw_buffer_event<addressT, POOL_BLOCK_SIZE>;

  // *** Data members
  ::channeler::packet_wrapper packet;

  // *** Constructor
  inline decrypted_packet_event(addressT const & source,
      addressT const & destination,
      ::channeler::packet_wrapper const & pkt,
      typename super_t::slot_type const & slot)
    : super_t{source, destination, slot}
    , packet{pkt}
  {
    // Explicitly override the type
    *const_cast<event_type *>(&(this->type)) = ET_DECRYPTED_PACKET;
  }


  virtual ~decrypted_packet_event() = default;
};



/**
 * The enqueued_packet_event extends the decrypted_packet_event by also
 * carrying a channel structure pointer, which is Nullable.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT
>
struct enqueued_packet_event
  : public decrypted_packet_event<addressT, POOL_BLOCK_SIZE>
{
  // *** Types
  using super_t = decrypted_packet_event<addressT, POOL_BLOCK_SIZE>;
  using channel_set = ::channeler::channels<channelT>;

  // *** Data members
  typename channel_set::channel_ptr channel;

  // *** Constructor
  inline enqueued_packet_event(addressT const & source,
      addressT const & destination,
      ::channeler::packet_wrapper const & pkt,
      typename super_t::slot_type const & slot,
      typename channel_set::channel_ptr const & ch)
    : super_t{source, destination, pkt, slot}
    , channel{ch}
  {
    // Explicitly override the type
    *const_cast<event_type *>(&(this->type)) = ET_ENQUEUED_PACKET;
  }


  virtual ~enqueued_packet_event() = default;
};


/**
 * Message events extend the enqueued_packet_event by a message_wrapper
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT
>
struct message_event
  : public enqueued_packet_event<addressT, POOL_BLOCK_SIZE, channelT>
{
  // *** Types
  using super_t = enqueued_packet_event<addressT, POOL_BLOCK_SIZE, channelT>;
  using message_type = typename messages::value_type;

  // *** Data members
  message_type message;

  // *** Constructor
  inline message_event(addressT const & source,
      addressT const & destination,
      ::channeler::packet_wrapper const & pkt,
      typename super_t::slot_type const & slot,
      typename super_t::channel_set::channel_ptr const & ch,
      message_type && msg)
    : super_t{source, destination, pkt, slot, ch}
    , message{std::move(msg)}
  {
    // Explicitly override the type
    *const_cast<event_type *>(&(this->type)) = ET_MESSAGE;
  }


  virtual ~message_event() = default;
};


/**
 * Outgoing messages
 */
template <typename addressT>
struct message_out_event
  : public event
{
  // *** Types
  using message_type = typename messages::value_type;

  // *** Data members
  peerid        sender;
  peerid        recipient;
  channelid     channel;
  message_type  message;

  // *** Constructor
  inline message_out_event(
      peerid const & _sender,
      peerid const & _recipient,
      channelid const & _channel,
      message_type && msg)
    : event{ET_MESSAGE_OUT}
    , sender{_sender}
    , recipient{_recipient}
    , channel{_channel}
    , message{std::move(msg)}
  {
  }


  virtual ~message_out_event() = default;
};


} // namespace channeler::pipe

#endif // guard
