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
#ifndef CHANNELER_CONTEXT_CONNECTION_H
#define CHANNELER_CONTEXT_CONNECTION_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>
#include <vector>

#include "../memory/packet_pool.h"
#include "../pipe/filter_classifier.h"


namespace channeler::context {

/**
 * The connection context is instanciated once per connection. It should be
 * as lightweight as possible.
 */
template <
  typename addressT,
  typename nodeT,
  typename transport_failure_policyT = ::channeler::pipe::null_policy<addressT>,
  typename peer_failure_policyT = ::channeler::pipe::null_policy<peerid_wrapper>
>
struct connection
{
  // *** Template type aliases
  using address_type = addressT;
  using node_type = nodeT;
  using transport_failure_policy_type = transport_failure_policyT;
  using peer_failure_policy_type = peer_failure_policyT;

  // *** Constants
  constexpr static std::size_t POOL_BLOCK_SIZE = node_type::POOL_BLOCK_SIZE;

  // *** Type aliases
  using channel_type = ::channeler::channel_data<
    POOL_BLOCK_SIZE,
    typename node_type::lock_policy_type
  >;
  using channel_set_type = ::channeler::channels<channel_type>;

  using timeouts_type = typename node_type::timeouts_type;
  using sleep_func = typename node_type::sleep_func;

  inline connection(node_type & node, peerid const & peer)
    : m_node{node}
    , m_peer{peer}
    , m_channels{}
    , m_timeouts{m_node.sleep()}
  {
  }

  inline peerid const & peer() const
  {
    return m_peer;
  }

  inline channel_set_type & channels()
  {
    return m_channels;
  }

  inline timeouts_type & timeouts()
  {
    return m_timeouts;
  }

  inline node_type & node()
  {
    return m_node;
  }

private:
  // *** Data members
  node_type &       m_node;
  peerid            m_peer;
  channel_set_type  m_channels;
  timeouts_type     m_timeouts;
};


} // namespace channeler::context

#endif // guard
