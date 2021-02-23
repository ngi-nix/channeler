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

#include "../lib/channels.h"

#include <gtest/gtest.h>

namespace {

struct channel
{
  inline channel(channeler::channelid const &)
  {
  }
};


} // anonymous namespace



TEST(Channels, empty_set)
{
  using namespace channeler;

  channels<channel> chs;

  channelid id = create_new_channelid();

  ASSERT_FALSE(chs.has_established_channel(id));
  ASSERT_FALSE(chs.has_pending_channel(id));
  ASSERT_FALSE(chs.has_channel(id));
}



TEST(Channels, add_bad_id_to_set)
{
  using namespace channeler;

  channels<channel> chs;

  // An identifier with just a responder is not valid
  channelid id;
  id.responder = 0xd00d;

  auto res = chs.add(id);

  ASSERT_EQ(ERR_INVALID_CHANNELID, res);
}



TEST(Channels, add_partial_to_set)
{
  using namespace channeler;

  channels<channel> chs;

  channelid id = create_new_channelid();

  EXPECT_FALSE(chs.has_established_channel(id));
  EXPECT_FALSE(chs.has_pending_channel(id));
  EXPECT_FALSE(chs.has_channel(id));

  // Now add a channel.
  auto res = chs.add(id);

  ASSERT_EQ(ERR_SUCCESS, res);

  ASSERT_FALSE(chs.has_established_channel(id));
  ASSERT_TRUE(chs.has_pending_channel(id));
  ASSERT_TRUE(chs.has_channel(id));
}



TEST(Channels, add_full_to_set)
{
  using namespace channeler;

  channels<channel> chs;

  channelid id = create_new_channelid();
  complete_channelid(id);

  EXPECT_FALSE(chs.has_established_channel(id));
  EXPECT_FALSE(chs.has_pending_channel(id));
  EXPECT_FALSE(chs.has_channel(id));

  // Now add a channel.
  auto res = chs.add(id);

  ASSERT_EQ(ERR_SUCCESS, res);

  ASSERT_TRUE(chs.has_established_channel(id));
  ASSERT_FALSE(chs.has_pending_channel(id));
  ASSERT_TRUE(chs.has_channel(id));
}



TEST(Channels, upgrade_partial)
{
  using namespace channeler;

  channels<channel> chs;

  // First add partial
  channelid id = create_new_channelid();

  auto res = chs.add(id);
  ASSERT_EQ(ERR_SUCCESS, res);

  auto ptr = chs.get(id);
  ASSERT_FALSE(ptr);

  // Try upgrading a partial
  res = chs.make_full(id);
  ASSERT_EQ(ERR_INVALID_CHANNELID, res);

  // Upgrade for real
  complete_channelid(id);

  res = chs.make_full(id);
  ASSERT_EQ(ERR_SUCCESS, res);

  ptr = chs.get(id);
  ASSERT_TRUE(ptr);
}



TEST(Channels, add_default_channel)
{
  // Adding the default channel should always result in a full, established
  // channel object being returned.

  using namespace channeler;

  channels<channel> chs;

  channelid id = DEFAULT_CHANNELID;
  auto res = chs.add(id);
  ASSERT_EQ(ERR_SUCCESS, res);

  ASSERT_TRUE(chs.has_established_channel(id));
  ASSERT_FALSE(chs.has_pending_channel(id));
  ASSERT_TRUE(chs.has_channel(id));

  auto ptr = chs.get(id);
  ASSERT_TRUE(ptr);
}
