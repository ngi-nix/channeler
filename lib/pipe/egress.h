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
#ifndef CHANNELER_PIPE_EGRESS_H
#define CHANNELER_PIPE_EGRESS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include "filter_classifier.h"

#include "egress/callback.h"
#include "egress/out_buffer.h"
#include "egress/add_checksum.h"
#include "egress/message_bundling.h"
#include "egress/enqueue_message.h"

#include "../channel_data.h"

namespace channeler::pipe {

/**
 * Default egress filter pipe.
 */
template <
  typename connection_contextT
>
struct default_egress
{
  // *** Basic context-derived type aliases and constants
  constexpr static std::size_t POOL_BLOCK_SIZE = connection_contextT::POOL_BLOCK_SIZE;
  using address_type = typename connection_contextT::address_type;

  using channel_type = typename connection_contextT::channel_type;
  using channel_set_type = typename connection_contextT::channel_set_type;
  using pool_type = typename connection_contextT::node_type::pool_type;
  using slot_type = typename pool_type::slot;



  // *** Filter type aliases
  using callback = callback_filter<
    channel_type
  >;
  using out_buffer = out_buffer_filter<
    address_type, POOL_BLOCK_SIZE,
    channel_type,
    callback, typename callback::input_event
  >;
  using add_checksum = add_checksum_filter<
    address_type, POOL_BLOCK_SIZE,
    out_buffer, typename out_buffer::input_event
  >;
  using message_bundling = message_bundling_filter<
    address_type, POOL_BLOCK_SIZE,
    channel_type,
    add_checksum, typename add_checksum::input_event
  >;
  using enqueue_message = enqueue_message_filter<
    channel_type,
    message_bundling, typename message_bundling::input_event
  >;

  using peerid_function = typename message_bundling::peerid_function;

  inline default_egress(
      typename callback::callback cb,
      channel_set_type & channels,
      pool_type & pool,
      peerid_function own_peerid_func,
      peerid_function peer_peerid_func
    )
    : m_callback{cb}
    , m_out_buffer{&m_callback, channels}
    , m_add_checksum{&m_out_buffer}
    , m_message_bundling{&m_add_checksum, channels, pool,
      own_peerid_func, peer_peerid_func}
    , m_enqueue_message{&m_message_bundling, channels}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    return m_enqueue_message.consume(std::move(ev));
  }

  callback          m_callback;
  out_buffer        m_out_buffer;
  add_checksum      m_add_checksum;
  message_bundling  m_message_bundling;
  enqueue_message   m_enqueue_message;
};


} // namespace channeler::pipe

#endif // guard
