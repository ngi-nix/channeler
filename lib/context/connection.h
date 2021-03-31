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


namespace channeler::context {

/**
 * The connection context is instanciated once per connection. It should be
 * as lightweight as possible.
 */
template <
  typename nodeT
>
struct connection
{
  // *** Template type aliases
  using node_type = nodeT;

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

  inline connection(node_type & node, peerid const & peer,
      sleep_func _sleep_func
    )
    : m_node{node}
    , m_peer{peer}
    , m_channels{}
    , m_timeouts{_sleep_func}
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
