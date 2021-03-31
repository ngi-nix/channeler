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

#include "../lib/pipe/ingress/validate.h"

#include <gtest/gtest.h>

#include "../../../packets.h"

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
    return {};
  }

  std::unique_ptr<channeler::pipe::event> m_event = {};
};

using simple_filter_t = channeler::pipe::validate_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event
>;


// Simple policy - rejects sender or recipient
template <
  typename addressT
>
struct reject_policy
{
  bool for_ingress;

  inline bool should_filter(addressT const &, bool ingress)
  {
    return (ingress == for_ingress);
  }
};

} // anonymous namespace



TEST(PipeIngressValidateFilter, throw_on_invalid_event)
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



TEST(PipeIngressValidateFilter, pass_packet)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  simple_filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
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



TEST(PipeIngressValidateFilter, drop_packet)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  data.data()[packet_default_channel_size - 1] = 0x00_b; // Bad checksum - null the last byte
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  simple_filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // We expect the event not to be passed on.
  ASSERT_FALSE(n.m_event);
}



TEST(PipeIngressValidateFilter, drop_packet_ban_transport_source)
{
  using namespace channeler::pipe;

  using peer_policy_t = reject_policy<channeler::peerid_wrapper>;
  using transport_policy_t = reject_policy<address_t>;
  using test_filter_t = channeler::pipe::validate_filter<
    address_t,
    POOL_BLOCK_SIZE,
    next,
    next::input_event,
    peer_policy_t,
    transport_policy_t
  >;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  data.data()[packet_default_channel_size - 1] = 0x00_b; // Bad checksum - null the last byte
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  transport_policy_t t{true};
  test_filter_t filter{&n, nullptr, &t};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  auto res = filter.consume(std::move(ev));

  // We expect the event not to be passed on.
  EXPECT_FALSE(n.m_event);

  // We also expect the result list to contain a recommendation to filter the
  // source address at the transport level
  ASSERT_EQ(1, res.size());
  auto & action = *res.begin();
  ASSERT_EQ(action->type, AT_FILTER_TRANSPORT);
  auto * ptr = reinterpret_cast<transport_filter_request_action<address_t> *>(action.get());
  ASSERT_EQ(ptr->address, 123);
  ASSERT_TRUE(ptr->ingress);
}



TEST(PipeIngressValidateFilter, drop_packet_ban_transport_destination)
{
  using namespace channeler::pipe;

  using peer_policy_t = reject_policy<channeler::peerid_wrapper>;
  using transport_policy_t = reject_policy<address_t>;
  using test_filter_t = channeler::pipe::validate_filter<
    address_t,
    POOL_BLOCK_SIZE,
    next,
    next::input_event,
    peer_policy_t,
    transport_policy_t
  >;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  data.data()[packet_default_channel_size - 1] = 0x00_b; // Bad checksum - null the last byte
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  transport_policy_t t{false};
  test_filter_t filter{&n, nullptr, &t};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  auto res = filter.consume(std::move(ev));

  // We expect the event not to be passed on.
  EXPECT_FALSE(n.m_event);

  // We also expect the result list to contain a recommendation to filter the
  // source address at the transport level
  ASSERT_EQ(1, res.size());
  auto & action = *res.begin();
  ASSERT_EQ(action->type, AT_FILTER_TRANSPORT);
  auto * ptr = reinterpret_cast<transport_filter_request_action<address_t> *>(action.get());
  ASSERT_EQ(ptr->address, 321);
  ASSERT_FALSE(ptr->ingress);
}



TEST(PipeIngressValidateFilter, drop_packet_ban_peer_sender)
{
  using namespace channeler::pipe;

  using peer_policy_t = reject_policy<channeler::peerid_wrapper>;
  using transport_policy_t = reject_policy<address_t>;
  using test_filter_t = channeler::pipe::validate_filter<
    address_t,
    POOL_BLOCK_SIZE,
    next,
    next::input_event,
    peer_policy_t,
    transport_policy_t
  >;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  data.data()[packet_default_channel_size - 1] = 0x00_b; // Bad checksum - null the last byte
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  peer_policy_t t{true};
  test_filter_t filter{&n, &t};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  auto res = filter.consume(std::move(ev));

  // We expect the event not to be passed on.
  EXPECT_FALSE(n.m_event);

  // We also expect the result list to contain a recommendation to filter the
  // source address at the transport level
  ASSERT_EQ(1, res.size());
  auto & action = *res.begin();
  ASSERT_EQ(action->type, AT_FILTER_PEER);
  auto * ptr = reinterpret_cast<peer_filter_request_action *>(action.get());
  ASSERT_EQ(ptr->peer, packet.sender());
  ASSERT_TRUE(ptr->ingress);
}



TEST(PipeIngressValidateFilter, drop_packet_ban_peer_recipient)
{
  using namespace channeler::pipe;

  using peer_policy_t = reject_policy<channeler::peerid_wrapper>;
  using transport_policy_t = reject_policy<address_t>;
  using test_filter_t = channeler::pipe::validate_filter<
    address_t,
    POOL_BLOCK_SIZE,
    next,
    next::input_event,
    peer_policy_t,
    transport_policy_t
  >;

  pool_type pool{PACKET_SIZE};

  // Copy packet data before parsing header
  auto data = pool.allocate();
  ::memcpy(data.data(), packet_default_channel, packet_default_channel_size);
  data.data()[packet_default_channel_size - 1] = 0x00_b; // Bad checksum - null the last byte
  channeler::packet_wrapper packet{data.data(), data.size()};

  next n;
  peer_policy_t t{false};
  test_filter_t filter{&n, &t};

  // No data added to event.
  auto ev = std::make_unique<simple_filter_t::input_event>(123, 321, packet, data);
  auto res = filter.consume(std::move(ev));

  // We expect the event not to be passed on.
  EXPECT_FALSE(n.m_event);

  // We also expect the result list to contain a recommendation to filter the
  // source address at the transport level
  ASSERT_EQ(1, res.size());
  auto & action = *res.begin();
  ASSERT_EQ(action->type, AT_FILTER_PEER);
  auto * ptr = reinterpret_cast<peer_filter_request_action *>(action.get());
  ASSERT_EQ(ptr->peer, packet.recipient());
  ASSERT_FALSE(ptr->ingress);
}
