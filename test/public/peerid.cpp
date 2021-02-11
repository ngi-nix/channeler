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

#include <channeler/peerid.h>

#include <gtest/gtest.h>

TEST(PeerID, default_constructed_random)
{
  auto id = channeler::peerid{};

  // While technically a peerid with all zeroes is random, we'll just discount
  // this as an unlikely case - if this test fails, that's the reason.
  size_t zerobytes = 0;
  for (size_t i = 0 ; i < channeler::PEERID_SIZE_BYTES ; ++i) {
    if (id.buffer[i] == std::byte{}) {
      ++zerobytes;
    }
  }
  ASSERT_LT(zerobytes, channeler::PEERID_SIZE_BYTES);
}



TEST(PeerID, default_constructed_unique)
{
  auto id1 = channeler::peerid{};
  auto id2 = channeler::peerid{};

  ASSERT_NE(id1, id2);
}



TEST(PeerID, copy_constructed)
{
  auto id1 = channeler::peerid{};
  auto id2 = id1;

  ASSERT_EQ(id1, id2);
}



TEST(PeerID, constructed_from_buffer)
{
  auto id1 = channeler::peerid{};
  auto id2 = channeler::peerid{id1.buffer, channeler::PEERID_SIZE_BYTES};

  ASSERT_EQ(id1, id2);
}



TEST(PeerID, construction_failure_from_buffer)
{
  std::byte * buf = nullptr;

  ASSERT_THROW((channeler::peerid{buf, 0}), std::out_of_range);
  ASSERT_THROW((channeler::peerid{buf, 1}), std::out_of_range);
}



TEST(PeerID, construction_failure_from_short_hex)
{
  char const * const foo = "0xd00d";
  ASSERT_THROW((channeler::peerid{foo, strlen(foo)}), std::out_of_range);

  char const * const bar = "0xthis-is-not-a-valid-hex-string-is-it-now?";
  ASSERT_THROW((channeler::peerid{bar, strlen(bar)}), std::invalid_argument);
}



TEST(PeerID, construct_from_hex)
{
  std::string test = "0xdeadd00ddeadd00ddeadd00ddeadd00d";
  channeler::peerid id{test.c_str(), test.size()};

  ASSERT_EQ(test, id.display());
}



TEST(PeerID, construct_from_bytes)
{
  std::string test = "0xdeadd00ddeadd00ddeadd00ddeadd00d";
  char const test_arr[] = {
    '\xde', '\xad', '\xd0', '\x0d',
    '\xde', '\xad', '\xd0', '\x0d',
    '\xde', '\xad', '\xd0', '\x0d',
    '\xde', '\xad', '\xd0', '\x0d',
  };

  channeler::peerid id{reinterpret_cast<std::byte const *>(test_arr),
    sizeof(test_arr)};

  ASSERT_EQ(test, id.display());
}



TEST(PeerID, default_constructed_hashes_unique)
{
  auto id1 = channeler::peerid{};
  auto id2 = channeler::peerid{};

  ASSERT_NE(id1.hash(), id2.hash());
}
