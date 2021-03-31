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

#include "ingress/state_handling.h"
#include "ingress/message_parsing.h"
#include "ingress/channel_assign.h"
#include "ingress/validate.h"
#include "ingress/route.h"
#include "ingress/de_envelope.h"

#include "../fsm/default.h"

namespace channeler::pipe {

/**
 * Default ingress filter pipe.
 */
template <
  typename connection_contextT
>
struct default_ingress
{

  // *** Basic context-derived type aliases and constants
  constexpr static std::size_t POOL_BLOCK_SIZE = connection_contextT::POOL_BLOCK_SIZE;
  using address_type = typename connection_contextT::address_type;

  using fsm_registry_type = ::channeler::fsm::registry;

  using channel_type = typename connection_contextT::channel_type;
  using channel_set_type = typename connection_contextT::channel_set_type;

  using peer_failure_policy_type = typename connection_contextT::peer_failure_policy_type;
  using transport_failure_policy_type = typename connection_contextT::transport_failure_policy_type;


  // *** Filter type aliases
  using state_handling = state_handling_filter<
    address_type, POOL_BLOCK_SIZE,
    channel_type,
    fsm_registry_type
  >;
  using message_parsing = message_parsing_filter<
    address_type, POOL_BLOCK_SIZE,
    state_handling, typename state_handling::input_event,
    channel_type,
    peer_failure_policy_type,
    transport_failure_policy_type
  >;
  using channel_assign = channel_assign_filter<
    address_type, POOL_BLOCK_SIZE,
    message_parsing, typename message_parsing::input_event,
    channel_type,
    peer_failure_policy_type,
    transport_failure_policy_type
  >;
  using validate = validate_filter<
    address_type, POOL_BLOCK_SIZE,
    channel_assign, typename channel_assign::input_event,
    peer_failure_policy_type,
    transport_failure_policy_type
  >;
  using route = route_filter<
    address_type, POOL_BLOCK_SIZE,
    validate, typename validate::input_event
  >;
  using de_envelope = de_envelope_filter<
    address_type, POOL_BLOCK_SIZE,
    route, typename route::input_event
  >;

  inline default_ingress(
      fsm_registry_type & registry,
      event_route_map & route_map,
      channel_set_type & channels,
      peer_failure_policy_type * peer_p = nullptr,
      transport_failure_policy_type * trans_p = nullptr
    )
    : m_state_handling{registry, route_map}
    , m_message_parsing{&m_state_handling}
    , m_channel_assign{&m_message_parsing, &channels, peer_p, trans_p}
    , m_validate{&m_channel_assign, peer_p, trans_p}
    , m_route{&m_validate}
    , m_de_envelope{&m_route}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    return m_de_envelope(std::move(ev));
  }

  state_handling  m_state_handling;
  message_parsing m_message_parsing;
  channel_assign  m_channel_assign;
  validate        m_validate;
  route           m_route;
  de_envelope     m_de_envelope;
};


} // namespace channeler::pipe

#endif // guard
