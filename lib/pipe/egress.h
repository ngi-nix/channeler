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
#ifndef CHANNELER_PIPE_INGRESS_H
#define CHANNELER_PIPE_INGRESS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include "filter_classifier.h"

#include "callback.h"
#include "out_buffer.h"
#include "add_checksum.h"
#include "message_bundling.h"

#include "../channel_data.h"

namespace channeler::pipe {

/**
 * Default egress filter pipe.
 */
template <
  typename contextT
>
struct default_egress
{
  // *** Basic context-derived type aliases and constants
  constexpr static std::size_t POOL_BLOCK_SIZE = contextT::POOL_BLOCK_SIZE;
  using address_type = typename contextT::address_type;

  using channel_type = typename contextT::channel_type;
  using channel_set = ::channeler::channels<channel_type>;


  // *** Filter type aliases
  using callback = callback_filter<
    channel_type
  >;
  using out_buffer = out_buffer_filter<
    address_type, POOL_BLOCK_SIZE,
    callback, typename callback::input_event,
    channel_type // TODO Move above next filter? Would be consistent with ingress filters
  >;
  using add_checksum = add_checksum_filter<
    address_type, POOL_BLOCK_SIZE,
    out_buffer, typename out_buffer::input_event
  >;
  using message_bundling = message_bundling_filter<
    address_type, POOL_BLOCK_SIZE,
    add_checksum, typename add_checksum::input_event,
    channel_type // TODO Move above next filter? Would be consistent with ingress filters
  >;


  inline default_egress(
      typename callback::callback cb,
      channel_set & channels
    )
    : m_callback{cb}
    , m_out_buffer{&m_callback, channels}
    , m_add_checksum{&m_out_buffer}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    // return m_de_envelope(std::move(ev));
  }

  callback      m_callback;
  out_buffer    m_out_buffer;
  add_checksum  m_add_checksum;
};


} // namespace channeler::pipe

#endif // guard
