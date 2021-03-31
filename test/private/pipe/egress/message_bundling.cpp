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

#include "../lib/pipe/egress/message_bundling.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
constexpr std::size_t PACKET_SIZE = 200;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;

struct next
{
  using input_event = channeler::pipe::packet_out_event<POOL_BLOCK_SIZE>;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    return {};
  }
  std::unique_ptr<channeler::pipe::event> m_event;
};


using filter_t = channeler::pipe::message_bundling_filter<
  address_t,
  POOL_BLOCK_SIZE,
  channeler::channel_data<POOL_BLOCK_SIZE>,
  next,
  next::input_event
>;

} // anonymous namespace



TEST(PipeIngressMessageBundlingFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};
  filter_t::channel_set chs;
  next n;
  filter_t filter{&n, chs, pool,
    []() { return channeler::peerid{}; },
    []() { return channeler::peerid{}; }
  };

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}


TEST(PipeIngressMessageBundlingFilter, bundle_message)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};
  filter_t::channel_set chs;
  next n;
  channeler::peerid self;
  channeler::peerid peer;
  filter_t filter{&n, chs, pool,
    [&self]() { return self; },
    [&peer]() { return peer; }
  };

  // Create input event
  channeler::peerid sender;
  channeler::peerid recipient;
  auto channel = channeler::create_new_channelid();
  channeler::complete_channelid(channel);
  auto err = chs.add(channel);
  EXPECT_EQ(channeler::ERR_SUCCESS, err);

  std::byte buf[130]; // It does not matter what's in this memory, but it
                      // does matter somewhat that the size of the buffer
                      // exceeds a length encodable in a single byte.
  auto msg = channeler::message_data::create(buf, sizeof(buf));
  auto ch = chs.get(channel);
  ch->enqueue_egress_message(std::move(msg));

  auto ev = std::make_unique<message_out_enqueued_event>(
      channel
  );
  auto ret = filter.consume(std::move(ev));

  ASSERT_EQ(0, ret.size());

  // No need to actually test packet header parsing - that's been tested
  // elsewhere. This tests that the filter passes on things well.
  ASSERT_EQ(n.m_event->type, ET_PACKET_OUT);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(self, ptr->packet.sender());
  ASSERT_EQ(peer, ptr->packet.recipient());
  ASSERT_EQ(channel, ptr->packet.channel());

  // We have one message type and two length bytes
  ASSERT_EQ(sizeof(buf) + 3, ptr->packet.payload_size());
}
