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
};


/**
 * Event base class
 */
struct event
{
  event_type const type = ET_UNKNOWN;
};


// TODO the input events of filters should be constructed from sharable
// templates, i.e. all filters accepting ET_DECRYPTED_PACKET can share
// code here.


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
};


} // namespace channeler::pipe

#endif // guard
