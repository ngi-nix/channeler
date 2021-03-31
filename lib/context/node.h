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
#ifndef CHANNELER_CONTEXT_NODE_H
#define CHANNELER_CONTEXT_NODE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>
#include <vector>

#include "../memory/packet_pool.h"
#include "../support/timeouts.h"

namespace channeler::context {

/**
 * The node context is instanciated once for a node, and *not* per
 * connection.
 *
 * Note: in future, if we want to support multiple peer identifiers per
 *       node, we may have to split this up further.
 */
template <
  std::size_t _POOL_BLOCK_SIZE,
  typename lock_policyT = null_lock_policy
>
struct node
{
  // *** Template type aliases
  using lock_policy_type = lock_policyT;

  // *** Constants
  constexpr static std::size_t POOL_BLOCK_SIZE = _POOL_BLOCK_SIZE;

  // *** Type aliases
  using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE, lock_policy_type>;
  using slot_type = typename pool_type::slot;

  using secret_type = std::vector<std::byte>;
  using secret_generator_type = std::function<secret_type ()>;

  // The timeouts *type* should be node defined, but the instance is per
  // connection.
  using timeouts_type = ::channeler::support::timeouts;
  using sleep_func = typename timeouts_type::sleep_function;

  inline node(peerid const & self, std::size_t packet_size,
      secret_generator_type generator
    )
    : m_self{self}
    , m_packet_size{packet_size}
    , m_packet_pool{packet_size}
    , m_secret_generator{generator}
  {
  }

  inline peerid const & id() const
  {
    return m_self;
  }

  inline std::size_t packet_size() const
  {
    return m_packet_size;
  }

  inline pool_type & packet_pool()
  {
    return m_packet_pool;
  }

  inline secret_generator_type & secret_generator()
  {
    return m_secret_generator;
  }

private:
  // *** Data members
  peerid                m_self;
  std::size_t           m_packet_size;
  pool_type             m_packet_pool;
  secret_generator_type m_secret_generator;
};


} // namespace channeler::context

#endif // guard
