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

#include "../lib/support/timeouts.h"

#include <gtest/gtest.h>

namespace {

// The test "sleep" function doesn't sleep, but pretends that the clock
// progressed exactly as much as requested.
channeler::support::timeouts::duration
test_sleep(channeler::support::timeouts::duration amount)
{
  return amount;
}

} // anonymous namepsace



TEST(SupportTimeouts, simple_timeout)
{
  channeler::support::timeouts to{&test_sleep};
  using duration = channeler::support::timeouts::duration;

  // Add a timeout
  auto ret = to.add({123, 321}, duration{10});
  ASSERT_TRUE(ret);

  // Progress the platform time
  auto exp = to.wait(duration{3});
  ASSERT_EQ(0, exp.size());

  // Progress another time
  exp = to.wait(duration{3});
  ASSERT_EQ(0, exp.size());

  // Progress again - this time, the 10 duration should expire
  exp = to.wait(duration{5});
  ASSERT_EQ(1, exp.size());

  ASSERT_EQ(exp[0].scope, 123);
  ASSERT_EQ(exp[0].tag, 321);
}



TEST(SupportTimeouts, duplicate_timeouts)
{
  channeler::support::timeouts to{&test_sleep};
  using duration = channeler::support::timeouts::duration;

  // Add a timeout
  auto ret = to.add({123, 321}, duration{10});
  ASSERT_TRUE(ret);

  // Cannot add the same timeout again, even with a different value.
  ret = to.add({123, 321}, duration{42});
  ASSERT_FALSE(ret);

  // However, a different scope or tag works.
  ret = to.add({124, 321}, duration{10});
  ASSERT_TRUE(ret);
  ret = to.add({123, 421}, duration{10});
  ASSERT_TRUE(ret);

  // Progress the platform time
  auto exp = to.wait(duration{10});
  ASSERT_EQ(3, exp.size());
}



TEST(SupportTimeouts, incremental_timeouts)
{
  channeler::support::timeouts to{&test_sleep};
  using duration = channeler::support::timeouts::duration;

  // Add timeouts
  auto ret = to.add({123, 321}, duration{10});
  ASSERT_TRUE(ret);
  ret = to.add({123, 421}, duration{11});
  ASSERT_TRUE(ret);

  auto exp = to.wait(duration{10});
  ASSERT_EQ(1, exp.size());

  // Just another unit more should provide the other timeout
  exp = to.wait(duration{1});
  ASSERT_EQ(1, exp.size());
}



TEST(SupportTimeouts, remove_timeouts)
{
  channeler::support::timeouts to{&test_sleep};
  using duration = channeler::support::timeouts::duration;

  // Add timeouts
  auto ret = to.add({123, 321}, duration{10});
  ASSERT_TRUE(ret);
  to.remove({123, 321});

  auto exp = to.wait(duration{10});
  ASSERT_EQ(0, exp.size());
}
