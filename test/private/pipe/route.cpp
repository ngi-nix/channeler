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

namespace {


inline constexpr std::byte operator "" _b(unsigned long long arg) noexcept
{
  return static_cast<std::byte>(arg);
}


std::byte const packet01[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier - all zeroes is the default channel
  0x00_b, 0x00_b, 0x00_b, 0x00_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0xfa_b, 0xfa_b, 0x25_b, 0xc3_b,

  // **** trailing dat
  // Spurious data in the buffer - this will be ignored
  0xde_b, 0xad_b, 0xbe_b, 0xef_b,
};



// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
constexpr std::size_t PACKET_SIZE = sizeof(packet01);

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;

struct next
{
  struct input_event : public channeler::pipe::event
  {
    inline input_event(address_t src, address_t dst, channeler::packet_wrapper p,
        pool_type::slot slot)
      : event{channeler::pipe::ET_PARSED_HEADER}
      , src_addr{src}
      , dst_addr{dst}
      , packet{p}
      , data{slot}
    {
    }

    address_t                 src_addr;
    address_t                 dst_addr;
    channeler::packet_wrapper packet;
    pool_type::slot           data;
  };

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    auto ptr = reinterpret_cast<input_event *>(m_event.get());
    channeler::pipe::action_list_type al;
    al.push_back(std::move(std::make_unique<channeler::pipe::peer_filter_request_action>(ptr->packet.sender())));
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
  ::memcpy(data.data(), packet01, sizeof(packet01));
  channeler::public_header_fields header{data.data()};

  next n;
  filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<filter_t::input_event>(123, 321, header, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_EQ(n.m_event->type, ET_PARSED_HEADER);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(123, ptr->src_addr);
  ASSERT_EQ(321, ptr->dst_addr);
  ASSERT_EQ(ptr->packet.sender().display(), "0x000000000000000000000000000a11c3");
  ASSERT_EQ(ptr->packet.recipient().display(), "0x00000000000000000000000000000b0b");
}



TEST(RouteFilter, drop_sender)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet01, sizeof(packet01));
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
  ::memcpy(data.data(), packet01, sizeof(packet01));
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
  ::memcpy(data1.data(), packet01, sizeof(packet01));
  channeler::public_header_fields header1{data1.data()};

  next n;
  filter_t filter{&n};

  // No data added to event.
  auto ev1 = std::make_unique<filter_t::input_event>(123, 321, header1, data1);
  auto res = filter.consume(std::move(ev1));

  // We expect the event to be passed on verbatim, so we'll test what there
  // is in the output event.
  ASSERT_EQ(res.size(), 1);
  EXPECT_EQ(n.m_event->type, ET_PARSED_HEADER);

  // However, the packet should have resulted in sender ban action from the
  // test next filter. That means a second packet with the same payload should
  // be dropped.
  n.m_event.reset();

  auto data2 = pool.allocate();
  ::memcpy(data2.data(), packet01, sizeof(packet01));
  channeler::public_header_fields header2{data2.data()};

  auto ev2 = std::make_unique<filter_t::input_event>(123, 321, header2, data2);
  res = filter.consume(std::move(ev2));

  ASSERT_EQ(res.size(), 0);
  ASSERT_FALSE(n.m_event);
}

