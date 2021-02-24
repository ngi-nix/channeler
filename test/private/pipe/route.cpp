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

#include "../lib/pipe/route.h"

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
  using input_event = channeler::pipe::decrypted_packet_event<address_t, POOL_BLOCK_SIZE>;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    auto ptr = reinterpret_cast<input_event *>(m_event.get());
    channeler::pipe::action_list_type al;
    al.push_back(std::make_shared<channeler::pipe::peer_filter_request_action>(ptr->packet.sender()));
    return al;
  }

  std::unique_ptr<channeler::pipe::event> m_event = {};
};

using filter_t = channeler::pipe::route_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event
>;

} // anonymous namespace

TEST(RouteFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  next n;
  filter_t filter{&n};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}



TEST(RouteFilter, pass_packet)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  channeler::public_header_fields header{data.data()};

  next n;
  filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<filter_t::input_event>(123, 321, header, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_EQ(n.m_event->type, ET_DECRYPTED_PACKET);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(123, ptr->transport.source);
  ASSERT_EQ(321, ptr->transport.destination);
  ASSERT_EQ(ptr->packet.sender().display(), "0x000000000000000000000000000a11c3");
  ASSERT_EQ(ptr->packet.recipient().display(), "0x00000000000000000000000000000b0b");
}



TEST(RouteFilter, drop_sender)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  channeler::public_header_fields header{data.data()};

  next n;
  filter_t filter{&n};

  // Add the sender to the sender banlist
  filter.m_sender_banlist.insert(header.sender.copy());

  // No data added to event.
  auto ev = std::make_unique<filter_t::input_event>(123, 321, header, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the next filter not to be called based on the filtered sender
  ASSERT_FALSE(n.m_event);
}



TEST(RouteFilter, drop_recipient)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  channeler::public_header_fields header{data.data()};

  next n;
  filter_t filter{&n};

  // Add the sender to the sender banlist
  filter.m_recipient_banlist.insert(header.recipient.copy());

  // No data added to event.
  auto ev = std::make_unique<filter_t::input_event>(123, 321, header, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the next filter not to be called based on the filtered sender
  ASSERT_FALSE(n.m_event);
}



TEST(RouteFilter, pass_first_drop_second)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data1 = pool.allocate();
  ::memcpy(data1.data(), packet_default_channel, packet_default_channel_size);
  channeler::public_header_fields header1{data1.data()};

  next n;
  filter_t filter{&n};

  // No data added to event.
  auto ev1 = std::make_unique<filter_t::input_event>(123, 321, header1, data1);
  auto res = filter.consume(std::move(ev1));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_EQ(res.size(), 1);
  EXPECT_EQ(n.m_event->type, ET_DECRYPTED_PACKET);

  // However, the packet should have resulted in sender ban action from the
  // test next filter. That means a second packet with the same payload should
  // be dropped.
  n.m_event.reset();

  auto data2 = pool.allocate();
  ::memcpy(data2.data(), packet_default_channel, packet_default_channel_size);
  channeler::public_header_fields header2{data2.data()};

  auto ev2 = std::make_unique<filter_t::input_event>(123, 321, header2, data2);
  res = filter.consume(std::move(ev2));

  ASSERT_EQ(res.size(), 0);
  ASSERT_FALSE(n.m_event);
}

