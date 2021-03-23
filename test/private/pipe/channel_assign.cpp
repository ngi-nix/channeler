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

#include "../lib/pipe/channel_assign.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

#include "../../packets.h"

using namespace test;

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
std::size_t PACKET_SIZE = packet_default_channel_size;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;


struct next
{
  using channel_data_t = channeler::channel_data<POOL_BLOCK_SIZE>;
  using input_event = channeler::pipe::enqueued_packet_event<address_t, POOL_BLOCK_SIZE, channel_data_t>;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    return {};
  }

  std::unique_ptr<channeler::pipe::event> m_event = {};
};



using simple_filter_t = channeler::pipe::channel_assign_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event,
  next::channel_data_t
>;

using channel_set = channeler::channels<next::channel_data_t>;


} // anonymous namespace



TEST(PipeChannelAssignFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  next n;
  channel_set chs{42};
  simple_filter_t filter{&n, &chs};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}



TEST(PipeChannelAssignFilter, pass_packet_default_channel)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  channel_set chs{42};
  simple_filter_t filter{&n, &chs};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_EQ(n.m_event->type, ET_ENQUEUED_PACKET);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(123, ptr->transport.source);
  ASSERT_EQ(321, ptr->transport.destination);
  ASSERT_EQ(ptr->packet.sender().display(), "0x000000000000000000000000000a11c3");
  ASSERT_EQ(ptr->packet.recipient().display(), "0x00000000000000000000000000000b0b");
}



TEST(PipeChannelAssignFilter, drop_packet_unknown_channel)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_regular_channelid, packet_regular_channelid_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  channel_set chs{42};
  simple_filter_t filter{&n, &chs};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_FALSE(n.m_event);
}


TEST(PipeChannelAssignFilter, pass_packet_known_channel)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_regular_channelid, packet_regular_channelid_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  channel_set chs{42};
  simple_filter_t filter{&n, &chs};

  // Before consuming the packet, make sure that the channel is already
  // known.
  chs.add(packet.channel());
  EXPECT_TRUE(chs.has_established_channel(packet.channel()));

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_TRUE(n.m_event);
  // XXX skip other verification, it's already covered above
}



TEST(PipeChannelAssignFilter, pass_packet_pending_channel)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_regular_channelid, packet_regular_channelid_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  channel_set chs{42};
  simple_filter_t filter{&n, &chs};

  // Before consuming the packet, make sure that the channel is already
  // known. We use a partial channel identifier, though.
  channeler::channelid id{packet.channel()};
  id.responder = channeler::DEFAULT_CHANNELID.responder;
  chs.add(id);
  EXPECT_TRUE(chs.has_pending_channel(id));

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the packet to be passed on without a channel structure. This is
  // to avoid creating buffers, but is necessary for support of early data.
  ASSERT_TRUE(n.m_event);
  ASSERT_EQ(n.m_event->type, ET_ENQUEUED_PACKET);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_FALSE(ptr->channel);
}
