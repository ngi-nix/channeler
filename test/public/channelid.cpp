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

#include <channeler/channelid.h>

#include <gtest/gtest.h>

TEST(ChannelID, default_constructed)
{
  auto id = channeler::channelid{};
  ASSERT_EQ(id, channeler::DEFAULT_CHANNELID);
  ASSERT_EQ(id.full, channeler::DEFAULT_CHANNELID.full);
  ASSERT_EQ(id.initiator, channeler::DEFAULT_CHANNELID.initiator);
  ASSERT_EQ(id.responder, channeler::DEFAULT_CHANNELID.responder);

  ASSERT_FALSE(id.has_initiator());
  ASSERT_FALSE(id.has_responder());
}


TEST(ChannelID, create)
{
  auto id = channeler::create_new_channelid();
  ASSERT_NE(id, channeler::DEFAULT_CHANNELID);
  ASSERT_TRUE(id.has_initiator());
  ASSERT_FALSE(id.has_responder());
}


TEST(ChannelID, complete_bad_initiator)
{
  auto id = channeler::DEFAULT_CHANNELID;
  ASSERT_EQ(channeler::ERR_INVALID_CHANNELID,
      channeler::complete_channelid(id));
}


TEST(ChannelID, complete_bad_responder)
{
  channeler::channelid id{};
  id.full = 0xdeadd00duL;
  ASSERT_EQ(channeler::ERR_INVALID_CHANNELID,
      channeler::complete_channelid(id));
}


TEST(ChannelID, complete)
{
  auto id = channeler::create_new_channelid();

  ASSERT_EQ(channeler::ERR_SUCCESS,
      channeler::complete_channelid(id));

  ASSERT_TRUE(id.has_initiator());
  ASSERT_TRUE(id.has_responder());
}
