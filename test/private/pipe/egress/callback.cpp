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

#include "../lib/pipe/egress/callback.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

#include "../../../packets.h"

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
constexpr std::size_t PACKET_SIZE = 200;

using channel_data = ::channeler::channel_data<
  POOL_BLOCK_SIZE
  // TODO lock policy
>;


using filter_t = channeler::pipe::callback_filter<
  channel_data
>;

} // anonymous namespace



TEST(PipeEgressCallbackFilter, pass_events)
{
  using namespace channeler::pipe;

  std::unique_ptr<event> caught;
  filter_t filter{
    [&caught](std::unique_ptr<event> ev) -> action_list_type
    {
      caught = std::move(ev);
      return {};
    }
  };

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();

  ASSERT_FALSE(caught);
  auto res = filter.consume(std::move(ev));
  ASSERT_TRUE(res.empty());
  ASSERT_TRUE(caught);
}
