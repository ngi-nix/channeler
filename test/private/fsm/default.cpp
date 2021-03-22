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

#include "../lib/fsm/default.h"
#include "../lib/context.h"

#include "../../temp_buffer.h"

#include <gtest/gtest.h>

namespace {

using packet_t = std::pair<
  channeler::packet_wrapper,
  test::temp_buffer
>;

inline packet_t
make_packet(channeler::peerid const & sender,
    channeler::peerid const & recipient)
{
  constexpr std::size_t packet_size =
    channeler::public_header_layout::PUB_SIZE
    + channeler::private_header_layout::PRIV_SIZE
    + channeler::footer_layout::FOOT_SIZE;

  test::temp_buffer buf{packet_size};

  channeler::packet_wrapper pkt{buf.buf.get(), buf.size};
  pkt.sender() = sender;
  pkt.recipient() = recipient;

  return std::make_pair(pkt, buf);
}

} // anonymous namespace


TEST(FSMDefaultRegistry, create)
{
  using namespace channeler::fsm;

  using ctx_t = channeler::default_context<
    int,
    3
  >;
  ctx_t ctx{
    42,
    [](channeler::support::timeouts::duration d) { return d; },
    []() -> std::vector<std::byte> { return {}; }
  };

  auto reg = get_standard_registry(ctx);
}


TEST(FSMDefaultRegistry, negotiate_channel)
{
  // We should be able to negotiate a channel between two default registries
  using namespace channeler::fsm;
  using namespace channeler::pipe;

  using ctx_t = channeler::default_context<
    int,
    3
  >;
  using pool_t = channeler::memory::packet_pool<
    ctx_t::POOL_BLOCK_SIZE
  >;
  using message_event_t = message_event<
    typename ctx_t::address_type,
    ctx_t::POOL_BLOCK_SIZE,
    typename ctx_t::channel_type
  >;

  pool_t pool{42};

  // Copying the context is fine here, we should just have two
  // unique contexts.
  ctx_t peer1_ctx{
    42,
    [](channeler::support::timeouts::duration d) { return d; },
    []() -> std::vector<std::byte> { return {}; }
  };
  ctx_t peer2_ctx = peer1_ctx;

  // Create two registries
  auto peer1_reg = get_standard_registry(peer1_ctx);
  auto peer2_reg = get_standard_registry(peer2_ctx);

  action_list_type actions;
  event_list_type events;
  bool result = false;

  // Step 1: one peer initiates a new channel
  //         We need to have a message event as the output.
  auto ev1 = new_channel_event(
      peer1_ctx.id, peer2_ctx.id);
  result = peer1_reg.process(&ev1, actions, events);
  ASSERT_TRUE(result);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto result_ev1 = *events.begin();
  ASSERT_TRUE(result_ev1);
  ASSERT_EQ(result_ev1->type, ET_MESSAGE_OUT);
  auto result_ev1_conv = reinterpret_cast<message_out_event *>(result_ev1.get());

  ASSERT_EQ(channeler::MSG_CHANNEL_NEW, result_ev1_conv->message->type);
  auto result_ev1_msg = reinterpret_cast<channeler::message_channel_new *>(result_ev1_conv->message.get());

  auto half_id = result_ev1_msg->initiator_part;
  ASSERT_TRUE(peer1_ctx.channels.has_pending_channel(half_id));
  ASSERT_FALSE(peer2_ctx.channels.has_pending_channel(half_id));

  // Step 2: the other peer returns a cookie.
  //         We have to feed the output message to the other peer's registry.
  auto pkt2 = make_packet(peer1_ctx.id, peer2_ctx.id);
  auto ev2 = message_event_t{123, 321, pkt2.first, pool.allocate(), {},
    std::move(result_ev1_conv->message)
  };

  actions.clear();
  events.clear();

  result = peer2_reg.process(&ev2, actions, events);
  ASSERT_TRUE(result);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto result_ev2 = *events.begin();
  ASSERT_TRUE(result_ev2);
  ASSERT_EQ(result_ev2->type, ET_MESSAGE_OUT);
  auto result_ev2_conv = reinterpret_cast<message_out_event *>(result_ev2.get());

  ASSERT_EQ(channeler::MSG_CHANNEL_ACKNOWLEDGE, result_ev2_conv->message->type);
  auto result_ev2_msg = reinterpret_cast<channeler::message_channel_acknowledge *>(result_ev2_conv->message.get());

  auto id = result_ev2_msg->id;
  ASSERT_EQ(half_id, id.initiator);
  ASSERT_TRUE(peer1_ctx.channels.has_pending_channel(id));
  ASSERT_FALSE(peer2_ctx.channels.has_pending_channel(id));

  // Step 3: the ack message should finalize the channel on peer1
  auto pkt3 = make_packet(peer2_ctx.id, peer1_ctx.id);
  auto ev3 = message_event_t{321, 123, pkt3.first, pool.allocate(), {},
    std::move(result_ev2_conv->message)
  };

  actions.clear();
  events.clear();

  result = peer1_reg.process(&ev3, actions, events);
  ASSERT_TRUE(result);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto result_ev3 = *events.begin();
  ASSERT_TRUE(result_ev3);
  ASSERT_EQ(result_ev3->type, ET_MESSAGE_OUT);
  auto result_ev3_conv = reinterpret_cast<message_out_event *>(result_ev3.get());

  ASSERT_EQ(channeler::MSG_CHANNEL_FINALIZE, result_ev3_conv->message->type);
  auto result_ev3_msg = reinterpret_cast<channeler::message_channel_finalize *>(result_ev3_conv->message.get());

  ASSERT_TRUE(peer1_ctx.channels.has_established_channel(id));
  ASSERT_FALSE(peer2_ctx.channels.has_pending_channel(id));

  // Step 4: peer 2 needs to process the finalize message to also have an
  //         established channel
  auto pkt4 = make_packet(peer1_ctx.id, peer2_ctx.id);
  auto ev4 = message_event_t{123, 321, pkt4.first, pool.allocate(), {},
    std::move(result_ev3_conv->message)
  };

  actions.clear();
  events.clear();

  result = peer2_reg.process(&ev4, actions, events);
  ASSERT_TRUE(result);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, events.size());

  ASSERT_TRUE(peer1_ctx.channels.has_established_channel(id));
  ASSERT_TRUE(peer2_ctx.channels.has_established_channel(id));
}