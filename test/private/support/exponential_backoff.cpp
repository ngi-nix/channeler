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

#include "../lib/support/exponential_backoff.h"

#include <map>

#include <gtest/gtest.h>

constexpr std::size_t ITERATIONS = 100;

TEST(SupportExponentialBackoff, simple_multiplier)
{
  // Basically validate the multiplier generation by ensuring it only produces
  // numbers between 0 and 1.
  std::map<std::size_t, std::size_t> results;
  for (std::size_t i = 0 ; i < ITERATIONS ; ++i) {
    results[channeler::support::backoff_multiplier(1)] += 1;
  }

  // We can have at most two results
  ASSERT_LE(results.size(), 2);
  for (auto e : results) {
    ASSERT_GE(e.first, 0);
    ASSERT_LE(e.first, 1);
    // std::cout << e.second << " x " << e.first << std::endl;
  }
}


TEST(SupportExponentialBackoff, larger_multiplier)
{
  // Basically validate the multiplier generation by ensuring it only produces
  // numbers between 0 and 7.
  std::map<std::size_t, std::size_t> results;
  for (std::size_t i = 0 ; i < ITERATIONS ; ++i) {
    results[channeler::support::backoff_multiplier(3)] += 1;
  }

  // We can have at most eight different results
  ASSERT_LE(results.size(), 8);
  for (auto e : results) {
    ASSERT_GE(e.first, 0);
    ASSERT_LE(e.first, 7);
    // std::cout << e.second << " x " << e.first << std::endl;
  }
}


TEST(SupportExponentialBackoff, backoff)
{
  // Same as large_multiplier above, but with an actual backoff value.
  std::map<int, std::size_t> results;
  for (std::size_t i = 0 ; i < ITERATIONS ; ++i) {
    results[channeler::support::backoff<int, int{42}>(3)] += 1;
  }

  // We can have at most eight different results
  ASSERT_LE(results.size(), 8);
  for (auto e : results) {
    ASSERT_GE(e.first, 0);
    ASSERT_LE(e.first, 7 * 42);
    // std::cout << e.second << " x " << e.first << std::endl;
  }
}
