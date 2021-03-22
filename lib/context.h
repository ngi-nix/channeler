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
#include "channel_data.h"
#include "channels.h"


namespace channeler {

/**
 * In a standard configuration, we need a default kind of context. This context
 * is provided here.
 */
template <
  typename addressT,
  std::size_t _POOL_BLOCK_SIZE
>
struct default_context
{
  using address_type = addressT;
  constexpr static std::size_t POOL_BLOCK_SIZE = _POOL_BLOCK_SIZE;
  using channel_type = channel_data<POOL_BLOCK_SIZE>;
  using channel_set_t = ::channeler::channels<channel_type>;

  using secret_type = std::vector<std::byte>;
  using secret_generator_t = std::function<secret_type ()>;

  using timeouts_t = ::channeler::support::timeouts;
  using sleep_function = typename timeouts_t::sleep_function;


  peerid              id;
  channel_set_t       channels;
  timeouts_t          timeouts;
  secret_generator_t  secret_generator;

  inline default_context(
      std::size_t packet_size,
      sleep_function && sleep,
      secret_generator_t generator
    )
    : channels{packet_size}
    , timeouts{std::forward<sleep_function>(sleep)}
    , secret_generator{generator}
  {
  }
};


} // namespace channeler

#endif // guard
