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

#include "../lib/fsm/channel_initiator.h"
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


TEST(FSMChannelInitiator, process_bad_event)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() {
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


TEST(FSMChannelInitiator, process_bad_message)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() { return fsm_t::secret_type{}; }};

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


TEST(FSMChannelInitiator, initiate_new_channel)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() { return fsm_t::secret_type{}; }};

  // Ok, let's create a new message event.
  peerid sender;
  peerid recipient;
  fsm_t::new_channel_event_type ev{sender, recipient};

  action_list_type actions;
  event_list_type events;
  auto ret = fsm.process(&ev, actions, events);

  // We expect that pretty much any new channel event will be processed
  // positively. This should produce an out event with a MSG_CHANNEL_NEW
  // entry.
  ASSERT_TRUE(ret);

  ASSERT_EQ(1, events.size());
  auto & rev = *events.begin();
  ASSERT_EQ(rev->type, ET_MESSAGE_OUT);
  auto rev_conv = reinterpret_cast<message_out_event *>(rev.get());
  ASSERT_TRUE(rev_conv->message);
  ASSERT_EQ(rev_conv->channel, DEFAULT_CHANNELID);
  ASSERT_EQ(rev_conv->message->type, MSG_CHANNEL_NEW);
  auto msg = reinterpret_cast<message_channel_new *>(rev_conv->message.get());
  ASSERT_NE(msg->initiator_part, DEFAULT_CHANNELID.initiator);

  // Also, our channel set must have an approriate pending channel.
  auto res = chs.get(msg->initiator_part);
  ASSERT_TRUE(res);
}


TEST(FSMChannelInitiator, timeout_pending_channel)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() { return fsm_t::secret_type{}; }};

  // Ok, let's create a new message event.
  peerid sender;
  peerid recipient;
  auto initiator = chs.new_pending_channel();

  action_list_type actions;
  event_list_type events;

  // With this processed, we'll want to ensure that the channel set
  // loses the channel if the FSM processes a timeout for the channel
  ASSERT_TRUE(chs.has_channel(initiator));
  fsm_t::timeout_event_type to_ev{{CHANNEL_NEW_TIMEOUT_TAG, initiator}};
  auto ret = fsm.process(&to_ev, actions, events);
  ASSERT_TRUE(ret);
  ASSERT_FALSE(chs.has_channel(initiator));
}


TEST(FSMChannelInitiator, acknowledge_channel)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() { return fsm_t::secret_type{}; }};

  // We don't want to create a new channel via the FSM. Just inject some
  // channel data into the channel set. We want a pending channel, so with
  // some initiator bits.
  peerid sender;
  peerid recipient;
  auto initiator = chs.new_pending_channel();

  action_list_type actions;
  event_list_type events;

  // With this processed, we want to acknowledge the channel. The acknowledgement
  // must have the same cookie as in the response message.
  fsm_t::secret_type secret;
  auto cookie = create_cookie_initiator(secret.data(), secret.size(),
      sender, recipient,
      initiator);

  // Need a packet buffer, even if the contents are not used
  test::temp_buffer data{test::packet_with_messages, test::packet_with_messages_size};
  channeler::packet_wrapper pkt{data.buf.get(), data.size};
  pkt.sender() = recipient;
  pkt.recipient() = sender;

  // Similarly, need a pool
  pool_type pool{TEST_PACKET_SIZE};
  auto ack = std::make_unique<message_channel_acknowledge>(
      channelid{initiator, 42},
      cookie,
      0xacab // cookie2
      );
  event_t ack_ev{123, 321, pkt, pool.allocate(), {},
    std::move(ack)
  };

  // Process this.
  ASSERT_TRUE(chs.has_pending_channel(initiator));
  ASSERT_FALSE(chs.has_established_channel(initiator));

  actions.clear();
  events.clear();
  auto ret = fsm.process(&ack_ev, actions, events);
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  ASSERT_FALSE(chs.has_pending_channel(initiator));
  ASSERT_TRUE(chs.has_established_channel(initiator));

  // We should also ensure that the out event is a message event, and contains
  // a message_channel_finalize with the appropriate cookie2
  auto & rev = *events.begin();
  ASSERT_EQ(rev->type, ET_MESSAGE_OUT);
  auto rev_conv = reinterpret_cast<message_out_event *>(rev.get());
  ASSERT_TRUE(rev_conv->message);
  ASSERT_EQ(rev_conv->channel, DEFAULT_CHANNELID);
  ASSERT_EQ(rev_conv->message->type, MSG_CHANNEL_FINALIZE);
  auto msg = reinterpret_cast<message_channel_finalize *>(rev_conv->message.get());
  ASSERT_EQ(msg->id, (channelid{initiator, 42}));
  ASSERT_EQ(msg->cookie2, 0xacab);
}


TEST(FSMChannelInitiator, timeout_established_channel)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;
  using namespace channeler::support;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_channel_initiator<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  timeouts t{[] (timeouts::duration a) -> timeouts::duration { return a; }};
  fsm_t fsm{t, chs, []() { return fsm_t::secret_type{}; }};

  // Ok, let's create a new message event.
  peerid sender;
  peerid recipient;
  auto initiator = chs.new_pending_channel();
  auto err = chs.make_full({initiator, 42});
  ASSERT_EQ(ERR_SUCCESS, err);

  action_list_type actions;
  event_list_type events;

  // With this processed, we'll want to ensure that the channel set
  // loses the channel if the FSM processes a timeout for the channel
  ASSERT_TRUE(chs.has_channel({initiator, 42}));
  fsm_t::timeout_event_type to_ev{{CHANNEL_TIMEOUT_TAG, initiator}};
  auto ret = fsm.process(&to_ev, actions, events);
  ASSERT_TRUE(ret);
  ASSERT_FALSE(chs.has_channel({initiator, 42}));
}

// TODO we can and should cover more branches of the FSM
