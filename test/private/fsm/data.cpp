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

#include "../lib/fsm/data.h"

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


TEST(FSMData, process_bad_event)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event ev;
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_FALSE(ret);
}


TEST(FSMData, process_bad_message)
{
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, {nullptr, 0, false}, pool.allocate(), {},
    parse_message(test::message_channel_cookie, test::message_channel_cookie_size)
  };
  auto ret = fsm.process(&ev, actions, events);
  ASSERT_FALSE(ret);
}


TEST(FSMData, remote_data_existing_channel)
{
  // - MSG_DATA on existing channel
  //  -> produce user data event
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // We need a packet with the above channel ID. Which won't exist, ever,
  // because we only have half a channel ID. But the important part is that
  // the initiator is the same.
  test::temp_buffer buf{test::packet_regular_channelid, test::packet_regular_channelid_size};
  packet_wrapper pkt{buf.buf.get(), buf.size};

  chs.add(pkt.channel());
  auto ch = chs.get(pkt.channel());

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, pkt, pool.allocate(), ch,
    parse_message(test::message_data, test::message_data_size)
  };
  auto ret = fsm.process(&ev, actions, events);

  // We processed it, but are discarding the results
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto & rev = *events.begin();
  ASSERT_TRUE(rev);
  ASSERT_EQ(ET_USER_DATA_TO_READ, rev->type);
  auto rev_conv = reinterpret_cast<user_data_to_read_event<TEST_POOL_BLOCK_SIZE> *>(rev.get());

  ASSERT_EQ(rev_conv->channel, pkt.channel());
  ASSERT_EQ(rev_conv->message->type, MSG_DATA);

  auto dmsg = reinterpret_cast<message_data *>(rev_conv->message.get());
  ASSERT_EQ(test::message_data_size, dmsg->buffer_size);

  for (size_t idx = 0 ; idx < dmsg->buffer_size ; ++idx) {
    ASSERT_EQ(test::message_data[idx], dmsg->buffer[idx]);
  }
}


TEST(FSMData, remote_data_pending_channel)
{
  // - MSG_DATA on pending channel
  //  -> ??? Should be prevented by previous filter's re-ordering
  //     of messages, but that is not implemented yet
  //  -> In context of the FSM, discard, possibly create a result
  //    action
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  auto cid = chs.new_pending_channel();
  auto ch = chs.get(cid);
  fsm_t fsm{chs};

  // We need a packet with the above channel ID. Which won't exist, ever,
  // because we only have half a channel ID. But the important part is that
  // the initiator is the same.
  test::temp_buffer buf{test::packet_regular_channelid, test::packet_regular_channelid_size};
  packet_wrapper pkt{buf.buf.get(), buf.size};
  pkt.channel().initiator = cid;

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, pkt, pool.allocate(), ch,
    parse_message(test::message_data, test::message_data_size)
  };
  auto ret = fsm.process(&ev, actions, events);

  // We processed it, but are discarding the results
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, events.size());
}


TEST(FSMData, remote_data_unknown_channel)
{
  // - MSG_DATA on unknown channel
  //  -> discard, possibly create a result action
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = message_event<int, TEST_POOL_BLOCK_SIZE, channel_t>;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  event_t ev{123, 321, {nullptr, 0, false}, pool.allocate(), {},
    parse_message(test::message_data, test::message_data_size)
  };
  auto ret = fsm.process(&ev, actions, events);

  // We processed it, but are discarding the results
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, events.size());
}


TEST(FSMData, local_data_existing_channel)
{
  // - User data on existing channel
  //  -> produce MSG_DATA event
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = fsm_t::data_written_event_type;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // Random channelid, make pending
  auto id = create_new_channelid();
  complete_channelid(id);
  chs.add(id);
  auto ch = chs.get(id);
  ASSERT_TRUE(ch);

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  std::vector<std::byte> data;
  event_t ev{id, data};
  auto ret = fsm.process(&ev, actions, events);

  // We processed it, but the result is an error
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto & out = *events.begin();
  ASSERT_TRUE(out);
  ASSERT_EQ(ET_MESSAGE_OUT, out->type);

  auto outconv = reinterpret_cast<message_out_event *>(out.get());
  ASSERT_EQ(id, outconv->channel);
}


TEST(FSMData, local_data_pending_channel)
{
  // - User data on pending channel
  //  -> channel_data.has_pending_output() must be true
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = fsm_t::data_written_event_type;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // Random channelid, make pending
  auto id = create_new_channelid();
  chs.add(id);
  auto ch = chs.get(id);
  ASSERT_TRUE(ch);

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  std::vector<std::byte> data;
  event_t ev{id, data};
  auto ret = fsm.process(&ev, actions, events);

  // Also for a pending channel we need to have an output event
  ASSERT_TRUE(ret);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(1, events.size());

  auto & out = *events.begin();
  ASSERT_TRUE(out);
  ASSERT_EQ(ET_MESSAGE_OUT, out->type);

  auto outconv = reinterpret_cast<message_out_event *>(out.get());
  ASSERT_EQ(id, outconv->channel);
}


TEST(FSMData, local_data_unknown_channel)
{
  // - User data on unknown channel
  //  -> error response of some sort
  using namespace channeler::fsm;
  using namespace channeler::pipe;
  using namespace channeler;

  // TODO I don't like having to pass the pool block size here at all.
  using channel_t = channel_data<TEST_POOL_BLOCK_SIZE>;
  using fsm_t = fsm_data<int, TEST_POOL_BLOCK_SIZE, channel_t>;
  using event_t = fsm_t::data_written_event_type;

  pool_type pool{TEST_PACKET_SIZE};
  fsm_t::channel_set chs;
  fsm_t fsm{chs};

  // Random channelid
  auto id = create_new_channelid();
  complete_channelid(id);

  // If we feed the FSM anything other than a ET_MESSAGE event, it will return
  // false.
  action_list_type actions;
  event_list_type events;
  std::vector<std::byte> data;
  event_t ev{id, data};
  auto ret = fsm.process(&ev, actions, events);

  // We processed it, but the result is an error
  ASSERT_TRUE(ret);
  ASSERT_EQ(1, actions.size());
  ASSERT_EQ(0, events.size());

  auto & act = *actions.begin();
  ASSERT_TRUE(act);
  ASSERT_EQ(AT_ERROR, act->type);

  auto convact = reinterpret_cast<error_action *>(act.get());
  ASSERT_EQ(ERR_INVALID_CHANNELID, convact->error);
}
