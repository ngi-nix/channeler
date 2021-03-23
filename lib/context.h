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
#ifndef CHANNELER_CONTEXT_H
#define CHANNELER_CONTEXT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>
#include <vector>

#include "support/timeouts.h"
#include "memory/packet_pool.h"
#include "channel_data.h"
#include "channels.h"
#include "lock_policy.h"


namespace channeler {

/**
 * In a standard configuration, we need a default kind of context. This context
 * is provided here.
 */
template <
  typename addressT,
  std::size_t _POOL_BLOCK_SIZE,
  typename lock_policyT = null_lock_policy
>
struct default_context
{
  // *** Constants
  constexpr static std::size_t POOL_BLOCK_SIZE = _POOL_BLOCK_SIZE;

  // *** Type aliases
  using address_type = addressT;
  using channel_type = channel_data<POOL_BLOCK_SIZE, lock_policyT>;
  using channel_set_type = ::channeler::channels<channel_type>;

  using secret_type = std::vector<std::byte>;
  using secret_generator_type = std::function<secret_type ()>;

  using timeouts_type = ::channeler::support::timeouts;
  using sleep_function_type = typename timeouts_type::sleep_function;

  using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE, lock_policyT>;

  // *** Data members
  peerid                id; // XXX Not per-connection, or is it?
  channel_set_type      channels;
  timeouts_type         timeouts; // XXX Not per-connection, or is it?
  secret_generator_type secret_generator; // XXX Not per-connection, or is it?
  pool_type             packet_pool; // XXX Not per-connection, or is it?

  // *** Ctor
  inline default_context(
      std::size_t packet_size,
      sleep_function_type && sleep,
      secret_generator_type generator
    )
    : channels{packet_size}
    , timeouts{std::forward<sleep_function_type>(sleep)}
    , secret_generator{generator}
    , packet_pool{packet_size}    // TODO lock policy?
  {
  }
};


} // namespace channeler

#endif // guard
