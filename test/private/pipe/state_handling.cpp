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

#include "../lib/pipe/state_handling.h"
#include "../lib/channel_data.h"
#include "../lib/context.h"
#include "../lib/fsm/default.h"

#include <gtest/gtest.h>

#include "../../packets.h"
#include "../../messages.h"


using namespace test;

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
std::size_t PACKET_SIZE = packet_with_messages_size;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;
using channel_data_t = channeler::channel_data<POOL_BLOCK_SIZE>;

using ctx_t = channeler::default_context<
  int,
  3
>;

using registry_t = channeler::fsm::registry<ctx_t>;

using simple_filter_t = channeler::pipe::state_handling_filter<
  address_t,
  POOL_BLOCK_SIZE,
  channel_data_t,
  registry_t
>;

using channel_set = channeler::channels<channel_data_t>;


struct event_capture
{
  std::unique_ptr<channeler::pipe::event> captured;

  channeler::pipe::action_list_type func(std::unique_ptr<channeler::pipe::event> ev)
  {
    captured = std::move(ev);
    return {};
  }
};


} // anonymous namespace


TEST(PipeStateHandlingFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  ctx_t ctx {
    200,
    [](ctx_t::timeouts_type::duration d) { return d; },
    []() -> std::vector<std::byte> { return {}; }
  };
  auto reg = channeler::fsm::get_standard_registry(ctx);
  event_route_map routemap;
  simple_filter_t filter{reg, routemap};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}


TEST(PipeStateHandlingFilter, create_message_on_channel_new)
{
  using namespace channeler::pipe;

  ctx_t ctx {
    200,
    [](ctx_t::timeouts_type::duration d) { return d; },
    []() -> std::vector<std::byte> { return {}; }
  };
  auto reg = channeler::fsm::get_standard_registry(ctx);
  event_route_map routemap;

  // Register callbacks; in this case we need to get an egress
  // message as a result.
  using namespace std::placeholders;
  event_capture cap_egress;
  routemap[EC_EGRESS] = std::bind(&event_capture::func, &cap_egress, _1);

  simple_filter_t filter{reg, routemap};

  // Copy packet data before parsing header
  pool_type pool{PACKET_SIZE};
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_regular_channelid, packet_regular_channelid_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  auto msg = std::make_unique<channeler::message>(
      test::message_channel_new, test::message_channel_new_size, true
  );
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data,
      channel_set::channel_ptr{},
      std::move(msg)
  );
  auto res = filter.consume(std::move(ev));

  // No actions
  ASSERT_EQ(0, res.size());

  // However, the egress capture function should have received an event.
  ASSERT_TRUE(cap_egress.captured);
  ASSERT_EQ(ET_MESSAGE_OUT, cap_egress.captured->type);
}
