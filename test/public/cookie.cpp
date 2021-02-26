/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2021 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright temp_buffer for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/

#include <channeler/cookie.h>

#include <gtest/gtest.h>

#include "../byte_suffix.h"

using namespace test;

namespace {

auto const secret1 = "s3kr1t"_b;
auto const secret2 = "t1rk3s"_b;

} // anonymous namespace


TEST(Cookie, initiator_cookie_calculation)
{
  using namespace channeler;

  peerid p1;
  peerid p2;

  channelid id = create_new_channelid();

  cookie c1 = create_cookie_initiator(secret1.data(), secret1.size(),
      p1, p2, id.initiator);

  ASSERT_NE(c1, cookie{});
  ASSERT_TRUE(validate_cookie(c1, secret1.data(), secret1.size(),
        p1, p2, id.initiator));
  ASSERT_FALSE(validate_cookie(c1 + 1, secret1.data(), secret1.size(),
        p1, p2, id.initiator));
}


TEST(Cookie, responder_cookie_calculation)
{
  using namespace channeler;

  peerid p1;
  peerid p2;

  channelid id = create_new_channelid();
  complete_channelid(id);

  cookie c2 = create_cookie_responder(secret2.data(), secret2.size(),
      p1, p2, id);

  ASSERT_NE(c2, cookie{});

  ASSERT_TRUE(validate_cookie(c2, secret2.data(), secret2.size(),
        p1, p2, id));
  ASSERT_FALSE(validate_cookie(c2 + 1, secret2.data(), secret2.size(),
        p1, p2, id));
}
