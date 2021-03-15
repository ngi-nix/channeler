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

#include "../lib/fsm/channel_responder.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

#include "../../messages.h"
#include "../../packets.h"
#include "../../temp_buffer.h"

namespace {

constexpr std::size_t TEST_PACKET_SIZE = 120;
constexpr std::size_t TEST_POOL_BLOCK_SIZE = 3;

using pool_type = ::channeler::memory::packet_pool<
  TEST_POOL_BLOCK_SIZE
>;

} // anonymous namespace


TEST(FSMChannelResponder, process_bad_event)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_responder<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs{TEST_PACKET_SIZE};
  fsm_t fsm{chs, []() {
    return fsm_t::secret_type{};
  }};

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event ev;
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_FALSE(ret);
}


TEST(FSMChannelResponder, process_bad_message)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<3>;
  using fsm_t = fsm_channel_responder<int, 3, channel_t>;
  using event_t = message_event<int, 3, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs{TEST_PACKET_SIZE};
  fsm_t fsm{chs, []() { return fsm_t::secret_type{}; }};

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, {nullptr, 0, false}, pool.allocate(), {},
    parse_message(test::message_data, test::message_data_size)
  };
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_FALSE(ret);
}



TEST(FSMChannelResponder, process_msg_channel_new)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace test;

  // We create the packet just so we have a packet slot for the FSM.
  // This is a little unfortunate, but we need the packet's sender and
  // recipient information to generate a response.
  temp_buffer data{packet_with_messages, packet_with_messages_size};
  channeler::packet_wrapper pkt{data.buf, data.size};

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<3>;
  using fsm_t = fsm_channel_responder<int, 3, channel_t>;
  using event_t = message_event<int, 3, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs{TEST_PACKET_SIZE};
  fsm_t fsm{chs, []() { return fsm_t::secret_type{}; }};

  // MSG_CHANNEL_NEW should be processed, and at this point we'll expect a
  // MSG_CHANNEL_ACKNOWLEDGE in return.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, pkt, pool.allocate(), {},
    parse_message(test::message_channel_new, test::message_channel_new_size)
  };
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto res = *events.begin();
  ASSERT_EQ(ET_MESSAGE_OUT, res->type);
  auto converted = reinterpret_cast<message_out_event *>(res.get());

  // The acknowledge message needs to swap sender and recipient, and be sent on
  // the deault channel.
  ASSERT_EQ(MSG_CHANNEL_ACKNOWLEDGE, converted->message->type);
  auto convmsg = reinterpret_cast<channeler::message_channel_acknowledge *>(converted->message.get());
  ASSERT_EQ(converted->sender, pkt.recipient());
  ASSERT_EQ(converted->recipient, pkt.sender());
  ASSERT_EQ(converted->channel, pkt.channel());

  // Check the cookie.
  auto secret = fsm_t::secret_type{};
  auto cookie = create_cookie_responder(secret.data(), secret.size(),
        pkt.sender(), pkt.recipient(), convmsg->id);
  ASSERT_EQ(cookie, convmsg->cookie2);
}


TEST(FSMChannelResponder, process_msg_channel_finalize)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace test;

  // We create the packet just so we have a packet slot for the FSM.
  // This is a little unfortunate, but we need the packet's sender and
  // recipient information to generate a response.
  temp_buffer data{packet_with_messages, packet_with_messages_size};
  channeler::packet_wrapper pkt{data.buf, data.size};

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<3>;
  using fsm_t = fsm_channel_responder<int, 3, channel_t>;
  using event_t = message_event<int, 3, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs{TEST_PACKET_SIZE};
  fsm_t fsm{chs, []() { return fsm_t::secret_type{}; }};

  // MSG_CHANNEL_FINALIZE should be processed, but we should not get output
  // events in return. However, our channel set should afterwards contain the
  // channel being sent.
  action_list_type actions;
  event_list_type events;
  auto msg = parse_message(test::message_channel_finalize, test::message_channel_finalize_size);
  auto convmsg = reinterpret_cast<channeler::message_channel_finalize *>(msg.get());
  auto expected_channel = convmsg->id;
  ASSERT_FALSE(chs.has_established_channel(expected_channel));

  event_t ev{123, 321, pkt, pool.allocate(), {}, std::move(msg)};
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, events.size());

  // We need to have the channel with the given channel identifier in the set
  // now.
  ASSERT_TRUE(chs.has_established_channel(expected_channel));
}
