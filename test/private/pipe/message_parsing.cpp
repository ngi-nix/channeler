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

#include "../lib/pipe/message_parsing.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

#include "../../packets.h"

using namespace test;

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
std::size_t PACKET_SIZE = packet_with_messages_size;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;


struct next
{
  using channel_data_t = channeler::channel_data<POOL_BLOCK_SIZE>;
  using input_event = channeler::pipe::message_event<address_t, POOL_BLOCK_SIZE, channel_data_t>;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_events.push_back(std::move(event));
    return {};
  }

  using list_t = std::list<
    std::unique_ptr<channeler::pipe::event>
  >;
  list_t m_events;
};



using simple_filter_t = channeler::pipe::message_parsing_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event,
  next::channel_data_t
>;

using channel_set = channeler::channels<next::channel_data_t>;


} // anonymous namespace


TEST(MessageParsingFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  next n;
  simple_filter_t filter{&n};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}



TEST(MessageParsingFilter, produce_message_events)
{
  // This test also tests that the slot reference counting mechanism works as
  // intended.

  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ASSERT_EQ(1, data.use_count());

  ::memcpy(data.data(), packet_with_messages, packet_with_messages_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  simple_filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data,
      channel_set::channel_ptr{});
  ASSERT_EQ(2, data.use_count());
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event. The data slot has a use count of 4 - one for the
  // data variable in this scope, and one each for every message event.
  ASSERT_EQ(3, n.m_events.size());
  ASSERT_EQ(4, data.use_count());

  for (auto & rev : n.m_events) {
    ASSERT_EQ(channeler::pipe::ET_MESSAGE, rev->type);
    next::input_event * re = reinterpret_cast<next::input_event *>(rev.get());
    ASSERT_NE(channeler::MSG_UNKNOWN, re->message->type);
  }
}



TEST(MessageParsingFilter, empty_packet)
{
  // This test also tests that the slot reference counting mechanism works as
  // intended.

  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ASSERT_EQ(1, data.use_count());

  ::memcpy(data.data(), packet_regular_channelid, packet_regular_channelid_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  simple_filter_t filter{&n};

  // No data added to event. Before "consume", the data should have a use count
  // of 2, because the slot is copied to the event. Afterwards, it should be 1
  // because the event has been consumed (move)
  ASSERT_EQ(1, data.use_count());
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data,
      channel_set::channel_ptr{});
  ASSERT_TRUE(ev);
  ASSERT_EQ(2, data.use_count());
  ASSERT_NO_THROW(filter.consume(std::move(ev)));
  ASSERT_FALSE(ev);
  ASSERT_EQ(1, data.use_count());

  // There should not be any events produced from this.
  ASSERT_EQ(0, n.m_events.size());
}
