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

#include "../lib/memory/packet_pool.h"

#include <gtest/gtest.h>

TEST(PacketPool, construction)
{
  using namespace channeler::memory;

  // A block size of 1 means that for every pool allocation, a new block
  // must be created.
  packet_pool<1> pool{42};;

  // Initially, we have no memory allocated for packets
  ASSERT_EQ(pool.capacity(), 0);
  ASSERT_TRUE(pool.empty());
}


TEST(PacketPool, allocation)
{
  using namespace channeler::memory;
  packet_pool<3> pool{42};;
  EXPECT_EQ(pool.capacity(), 0);
  EXPECT_TRUE(pool.empty());

  // Allocating a single item must increase the capacity
  // from zero to the block size.
  auto slot = pool.allocate();
  ASSERT_EQ(pool.capacity(), 3);
  ASSERT_EQ(pool.size(), 1);
  ASSERT_FALSE(pool.empty());

  // Also the slot should be valid
  ASSERT_EQ(slot.size(), 42);
  ASSERT_NE(slot.data(), nullptr);
}


TEST(PacketPool, deallocation)
{
  using namespace channeler::memory;
  packet_pool<3> pool{42};;
  EXPECT_EQ(pool.capacity(), 0);
  EXPECT_TRUE(pool.empty());

  // Allocating a single item must increase the capacity
  // from zero to the block size.
  auto slot = pool.allocate();
  EXPECT_EQ(pool.capacity(), 3);
  EXPECT_EQ(pool.size(), 1);
  EXPECT_FALSE(pool.empty());

  // Also the slot should be valid
  EXPECT_EQ(slot.size(), 42);
  EXPECT_NE(slot.data(), nullptr);

  // Freeing the slot should return things back to normal.
  pool.free(slot);

  ASSERT_EQ(slot.size(), 0);
  ASSERT_EQ(slot.data(), nullptr);

  ASSERT_EQ(pool.capacity(), 3);
  ASSERT_EQ(pool.size(), 0);
  ASSERT_TRUE(pool.empty());
}


TEST(PacketPool, deallocation_automatic)
{
  using namespace channeler::memory;
  packet_pool<3> pool{42};;
  EXPECT_EQ(pool.capacity(), 0);
  EXPECT_TRUE(pool.empty());

  // Automatic deallocation means that if the allocated slot goes out of scope,
  // we'll have it freed. We'll use nested scopes to test this.
  {
    // Allocate in first scope
    auto slot = pool.allocate();
    EXPECT_EQ(pool.capacity(), 3);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_FALSE(pool.empty());
    EXPECT_EQ(slot.size(), 42);
    EXPECT_NE(slot.data(), nullptr);

    {
      // Copy in second scope - this does not change anything about the pool
      auto slot2 = slot;

      ASSERT_EQ(pool.capacity(), 3);
      ASSERT_EQ(pool.size(), 1);
      ASSERT_FALSE(pool.empty());
    }

    // Copy is out of scope now - again, no pool changes
    ASSERT_EQ(pool.capacity(), 3);
    ASSERT_EQ(pool.size(), 1);
    ASSERT_FALSE(pool.empty());
  }

  // Completely out of scope - slot should be freed.
  ASSERT_EQ(pool.capacity(), 3);
  ASSERT_EQ(pool.size(), 0);
  ASSERT_TRUE(pool.empty());
}


namespace {

template <typename poolT>
inline void prune_helper(
    poolT & pool,
    typename poolT::slot & slot1,
    typename poolT::slot & slot2
)
{

  // Capacity and size of the pool must both be at 2
  ASSERT_EQ(pool.capacity(), 2);
  ASSERT_EQ(pool.size(), 2);

  // Now freeing a slot - either of them - should reduce the size, but leave
  // the capacity intact.
  pool.free(slot1);
  ASSERT_EQ(pool.capacity(), 2);
  ASSERT_EQ(pool.size(), 1);

  // Pruning should return the capacity to the same as the size.
  pool.prune();
  ASSERT_EQ(pool.capacity(), 1);
  ASSERT_EQ(pool.size(), 1);

  // Same again with the other slot
  pool.free(slot2);
  ASSERT_EQ(pool.capacity(), 1);
  ASSERT_EQ(pool.size(), 0);

  pool.prune();
  ASSERT_EQ(pool.capacity(), 0);
  ASSERT_EQ(pool.size(), 0);
}


} // anonymous namespace


TEST(PacketPool, pruning_ascending)
{
  using namespace channeler::memory;
  packet_pool<1> pool{42};;
  EXPECT_EQ(pool.capacity(), 0);
  EXPECT_TRUE(pool.empty());

  // We allocate two slots - since each block is only one slot in size, this
  // must affect the block count.
  auto slot1 = pool.allocate();
  auto slot2 = pool.allocate();

  prune_helper(pool, slot1, slot2);
}


TEST(PacketPool, pruning_descending)
{
  using namespace channeler::memory;
  packet_pool<1> pool{42};;
  EXPECT_EQ(pool.capacity(), 0);
  EXPECT_TRUE(pool.empty());

  // We allocate two slots - since each block is only one slot in size, this
  // must affect the block count.
  auto slot1 = pool.allocate();
  auto slot2 = pool.allocate();

  prune_helper(pool, slot2, slot1);
}
