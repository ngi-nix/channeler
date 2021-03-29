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
 * Events are pushed down the filter pipe. They have a type, a category, and
 * may carry a type-dependent payload.
 *
 * We have two filter pipes, an ingress and an egress filter pipe. Messages
 * from a peer are processed in the ingress filter pipe, and responses are
 * processed in the egress filter pipe.
 *
 * Additionally, we have events that are triggered from user interactions,
 * or from system events (e.g. timeouts). They are not strictly speaking
 * part of the filter pipe, but may be handled in a state machine.
 *
 * Finally, we have events that are produced by state machines to notify the
 * user in some way - these are distrinct from events intended for the egress
 * filter pipe.
 *
 * We classify these events into categories.
 * TODO: https://gitlab.com/interpeer/channeler/-/issues/22
 *       Also, not all event types are closely related to the pipe...
 *       we should move the base class, type and category outside of
 *       this directory.
 */
enum event_category : uint_fast16_t
{
  EC_UNKNOWN = 0,   // Do not use

  EC_INGRESS,       // From I/O; packets, messages, etc.
  EC_EGRESS,        // To I/O; packages, messages, etc.
  EC_USER,          // From user action, e.g. "create channel"
  EC_SYSTEM,        // From system, e.g. timeouts
  EC_NOTIFICATION,  // To user, e.g. "an error occurred"
};

// TODO improve naming
enum event_type : uint_fast16_t
{
  ET_UNKNOWN = 0, // Do not use

  // ** EC_INGRESS
  ET_RAW_BUFFER,
  ET_PARSED_HEADER,
  // TODO encrypted or potentially encrypted packet might have a different
  // event type.
  ET_DECRYPTED_PACKET,
  ET_ENQUEUED_PACKET,
  ET_MESSAGE,

  // ** EC_EGRESS
  ET_MESSAGE_OUT,
  ET_MESSAGE_OUT_ENQUEUED,
  ET_PACKET_OUT,          // Contains an outbound/egress packet
  ET_PACKET_OUT_ENQUEUED, // Same, but added to an output buffer

  // ** EC_USER
  ET_NEW_CHANNEL,       // User creates new channel
  ET_USER_DATA_WRITTEN, // User writes data (to channel)

  // ** EC_SYSTEM
  ET_TIMEOUT,

  // ** EC_NOTIFICATION
  ET_USER_DATA_TO_READ, // User should be notified that there is data to read (from channel)
  ET_ERROR, // Error event (usually usage error)
};


/**
 * Event base class
 */
struct event
{
  event_category const  category;
  event_type const      type;

  inline event(event_category c = EC_UNKNOWN, event_type t = ET_UNKNOWN)
    : category{c}
    , type{t}
  {
  }

  virtual ~event() = default;
};

// Event list type
using event_list_type = std::list<std::unique_ptr<event>>;


// TODO pretty much all of the events carrying packet data can be merged into
//      one class, and have just the type change & asserted in each filter.
//      timeouts, etc. would need another class.
//      https://gitlab.com/interpeer/channeler/-/issues/22


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
    : event{EC_INGRESS, ET_RAW_BUFFER}
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
struct message_out_event
  : public event
{
  // *** Types
  using message_type = typename messages::value_type;

  // *** Data members
  channelid     channel;
  message_type  message;

  // *** Constructor
  inline message_out_event(
      channelid const & _channel,
      message_type && msg)
    : event{EC_EGRESS, ET_MESSAGE_OUT}
    , channel{_channel}
    , message{std::move(msg)}
  {
  }


  virtual ~message_out_event() = default;
};


/**
 * Outgoing messages
 */
struct message_out_enqueued_event
  : public event
{
  // *** Data members
  channelid     channel;

  // *** Constructor
  inline message_out_enqueued_event(channelid const & _channel)
    : event{EC_EGRESS, ET_MESSAGE_OUT_ENQUEUED}
    , channel{_channel}
  {
  }


  virtual ~message_out_enqueued_event() = default;
};


/**
 * Outgoing packets
 */
template <
  std::size_t POOL_BLOCK_SIZE
>
struct packet_out_event
  : public event
{
  // *** Types
  using pool_type = ::channeler::memory::packet_pool<
    POOL_BLOCK_SIZE
    // FIXME lock policy
  >;
  using slot_type = typename pool_type::slot;

  // *** Data members
  slot_type                   slot;
  ::channeler::packet_wrapper packet;

  // *** Constructor
  inline packet_out_event(
      slot_type && _slot,
      ::channeler::packet_wrapper && _packet
    )
    : event{EC_EGRESS, ET_PACKET_OUT}
    , slot{std::move(_slot)}
    , packet{std::move(_packet)}
  {
  }


  virtual ~packet_out_event() = default;
};


/**
 * Outgoing packets (enqueued)
 */
template <
  typename channelT
>
struct packet_out_enqueued_event
  : public event
{
  // *** Types
  using channel_set = ::channeler::channels<channelT>;
  using channel_ptr = typename channel_set::channel_ptr;

  // *** Data members
  channel_ptr                 channel;

  // *** Constructor
  inline packet_out_enqueued_event(
      channel_ptr _channel
    )
    : event{EC_EGRESS, ET_PACKET_OUT_ENQUEUED}
    , channel{_channel}
  {
  }


  virtual ~packet_out_enqueued_event() = default;
};



/**
 * Event for a new channel
 */
struct new_channel_event
  : public event
{
  // *** Data members
  peerid        sender;
  peerid        recipient;

  inline new_channel_event(
      peerid const & _sender,
      peerid const & _recipient)
    : event{EC_USER, ET_NEW_CHANNEL}
    , sender{_sender}
    , recipient{_recipient}
  {
  }

  virtual ~new_channel_event() = default;
};


/**
 * Event for timeouts. Carries some kind of usage specific context, but has
 * specialization for being empty.
 */
template <typename contextT = void>
struct timeout_event
  : public event
{
  // *** Data members
  contextT  context;

  inline timeout_event(
      contextT const & ctx)
    : event{EC_SYSTEM, ET_TIMEOUT}
    , context{ctx}
  {
  }

  virtual ~timeout_event() = default;
};

template <>
struct timeout_event<void>
  : public event
{
  inline timeout_event()
    : event{EC_SYSTEM, ET_TIMEOUT}
  {
  }

  virtual ~timeout_event() = default;
};


/**
 * Events for user data.
 */
struct user_data_written_event
  : public event
{
  channelid               channel;
  // TODO would be nice to have a pool reference instead of an allocation here,
  //      but we can deal with that later.
  std::vector<std::byte>  data;

  inline user_data_written_event(
      channelid const & _channel,
      std::vector<std::byte> const & _data)
    : event{EC_USER, ET_USER_DATA_WRITTEN}
    , channel{_channel}
    , data{_data}
  {
  }

  virtual ~user_data_written_event() = default;
};




template <
  std::size_t POOL_BLOCK_SIZE
>
struct user_data_to_read_event
  : public event
{
  // *** Types
  using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;
  using slot_type = typename pool_type::slot;
  using message_type = typename messages::value_type;

  // *** Data members
  channelid     channel;
  slot_type     slot;
  message_type  message;

  // *** Constructor
  inline user_data_to_read_event(
      channelid const & _channel,
      slot_type const & _slot,
      message_type && _msg)
    : event{EC_NOTIFICATION, ET_USER_DATA_TO_READ}
    , channel{_channel}
    , slot{_slot}
    , message{std::move(_msg)}
  {
  }

  virtual ~user_data_to_read_event() = default;
};



/**
 * Error event
 */
struct error_event
  : public event
{
  // *** Data members
  error_t error;

  inline error_event(error_t err)
    : event{EC_NOTIFICATION, ET_ERROR}
    , error{err}
  {
  }

  virtual ~error_event() = default;
};




} // namespace channeler::pipe

#endif // guard
