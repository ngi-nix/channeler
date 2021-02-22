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
#ifndef CHANNELER_PIPE_DE_ENVELOPE_H
#define CHANNELER_PIPE_DE_ENVELOPE_H

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
 * The de-envelope filter raw buffers, parses packet headers, and passes on the
 * result.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT
>
struct de_envelope_filter
{
  using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;
  using slot_type = typename pool_type::slot;

  struct input_event : public event
  {
    addressT  src_addr;
    addressT  dst_addr;
    slot_type data;

    inline input_event(addressT const & src, addressT const & dst,
        slot_type const & slot)
      : event{ET_RAW_BUFFER}
      , src_addr{src}
      , dst_addr{dst}
      , data{slot}
    {
    }
  };


  de_envelope_filter(next_filterT * next)
    : m_next{next}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_RAW_BUFFER) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event const * in = reinterpret_cast<input_event const *>(ev.get());

    // If there is no data passed, we should also throw.
    if (nullptr == in->data.data()) {
      throw exception{ERR_INVALID_REFERENCE};
    }

    // Parse header data, and pass on a new event to the next filter
    ::channeler::public_header_fields header{in->data.data()};

    auto next = std::make_unique<next_eventT>(
        in->src_addr, in->dst_addr, header, in->data);
    return m_next->consume(std::move(next));
  }


  next_filterT *  m_next;
};


} // namespace channeler::pipe

#endif // guard
