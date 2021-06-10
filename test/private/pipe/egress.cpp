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

#include "../lib/pipe/egress.h"
#include "../lib/context/node.h"
#include "../lib/context/connection.h"

#include <gtest/gtest.h>

namespace {

constexpr static std::size_t PACKET_SIZE = 300;
constexpr static std::size_t POOL_BLOCK_SIZE = 3;

using address_t = int;

using node_t = ::channeler::context::node<
  POOL_BLOCK_SIZE
  // XXX lock policy is null by default
>;

using connection_t = ::channeler::context::connection<
  address_t,
  node_t
>;

using egress_t = channeler::pipe::default_egress<
  connection_t
>;


} // anonymous namespace


TEST(PipeEgress, create)
{
  using namespace channeler::pipe;

  channeler::peerid self;
  channeler::peerid peer;

  node_t node{
    self,
    PACKET_SIZE,
    []() -> std::vector<channeler::byte> { return {}; },
    [](channeler::support::timeouts::duration d) { return d; },
  };

  connection_t ctx{
    node,
    peer
  };

  typename egress_t::channel_set_type chs;
  typename egress_t::pool_type pool{PACKET_SIZE};

  std::unique_ptr<event> caught;
  egress_t egress{
    [&caught](std::unique_ptr<event> ev) -> action_list_type
    {
      caught = std::move(ev);
      return {};
    },
    chs,
    pool,
    []() -> channeler::peerid { return {}; },
    []() -> channeler::peerid { return {}; }
  };
}
