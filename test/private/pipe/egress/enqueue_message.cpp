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

#include "../lib/pipe/egress/enqueue_message.h"
#include "../lib/channel_data.h"

#include <gtest/gtest.h>

namespace {

constexpr std::size_t PACKET_SIZE = 200;
constexpr std::size_t POOL_BLOCK_SIZE = 3;

struct next
{
  using input_event = channeler::pipe::message_out_enqueued_event;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    return {};
  }
  std::unique_ptr<channeler::pipe::event> m_event;
};


using filter_t = channeler::pipe::enqueue_message_filter<
  ::channeler::channel_data<POOL_BLOCK_SIZE>,
  next,
  next::input_event
>;

} // anonymous namespace



TEST(PipeEgressEnqueueMessageFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  next n;
  filter_t::channel_set chs;
  filter_t filter{&n, chs};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}


TEST(PipeEgressEnqueueMessageFilter, enqueue_message)
{
  using namespace channeler::pipe;

  next n;
  filter_t::channel_set chs;
  filter_t filter{&n, chs};

  auto channel = channeler::create_new_channelid();
  channeler::complete_channelid(channel);
  auto err = chs.add(channel);
  EXPECT_EQ(channeler::ERR_SUCCESS, err);
  auto ch = chs.get(channel);

  std::byte buf[130]; // It does not matter what's in this memory, but it
                      // does matter somewhat that the size of the buffer
                      // exceeds a length encodable in a single byte.
  auto msg = channeler::message_data::create(buf, sizeof(buf));
  auto ev = std::make_unique<message_out_event>(
      channel,
      std::move(msg)
  );
  ASSERT_FALSE(ch->has_egress_data_pending());
  auto ret = filter.consume(std::move(ev));
  ASSERT_TRUE(ch->has_egress_data_pending());

  ASSERT_EQ(0, ret.size());

  // Ensure the message has been moved.
  ASSERT_TRUE(n.m_event);
  ASSERT_EQ(n.m_event->type, ET_MESSAGE_OUT_ENQUEUED);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(channel, ptr->channel);

  auto res = ch->dequeue_egress_message();
  ASSERT_TRUE(res);
  ASSERT_EQ(channeler::MSG_DATA, res->type);
  auto resconv = reinterpret_cast<channeler::message_data *>(res.get());
  ASSERT_EQ(resconv->payload_size, sizeof(buf));

  for (std::size_t i = 0 ; i < resconv->payload_size ; ++i) {
    ASSERT_EQ(buf[i], resconv->payload[i]);
  }
}
