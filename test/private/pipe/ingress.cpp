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

#include "../lib/pipe/ingress.h"
#include "../lib/context/node.h"
#include "../lib/context/connection.h"

#include <gtest/gtest.h>

namespace {

constexpr static std::size_t PACKET_SIZE = 300;
constexpr std::size_t POOL_BLOCK_SIZE = 3;

using address_t = int;

using node_t = ::channeler::context::node<
  POOL_BLOCK_SIZE
  // XXX lock policy is null by default
>;

using connection_t = ::channeler::context::connection<
  address_t,
  node_t
>;

using ingress_t = channeler::pipe::default_ingress<
  connection_t
>;


} // anonymous namespace


TEST(PipeIngress, create)
{
  using namespace channeler::pipe;

  channeler::peerid self;
  channeler::peerid peer;

  node_t node{
    self,
    PACKET_SIZE,
    []() -> std::vector<std::byte> { return {}; },
    [](channeler::support::timeouts::duration d) { return d; },
  };

  connection_t ctx{
    node,
    peer
  };


  auto registry = channeler::fsm::get_standard_registry<address_t>(ctx);
  typename ingress_t::channel_set_type chs;
  event_route_map route_map;

  ingress_t ingress{registry, route_map, chs};
}
