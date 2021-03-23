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

#include "../lib/memory/packet_block.h"

#include <gtest/gtest.h>

TEST(MemoryPacketBlock, memory_footprint)
{
  using namespace channeler::memory;
  packet_block<3> block{42};

  // This set of 'tests' really just shows the memory footprint associated with
  // managing the free list. It doesn't test anything as such.
  ASSERT_EQ(block.capacity(), 3);
  ASSERT_EQ(block.packet_size(), 42);
  ASSERT_EQ(block.memory_size(), 42 * 3);
}


TEST(MemoryPacketBlock, freelist_after_construction)
{
  using namespace channeler::memory;
  packet_block<3> block{42};

  ASSERT_EQ(block.avail(), 3);
  ASSERT_EQ(block.size(), 0);
  ASSERT_TRUE(block.empty());
  ASSERT_FALSE(block.full());
}


TEST(MemoryPacketBlock, allocation)
{
  using namespace channeler::memory;
  packet_block<2> block{42};

  EXPECT_EQ(block.avail(), 2);
  EXPECT_EQ(block.size(), 0);
  EXPECT_TRUE(block.empty());
  EXPECT_FALSE(block.full());

  // First allocation is good.
  auto slot = block.allocate();
  ASSERT_EQ(slot.size(), 42);
  ASSERT_NE(slot.data(), nullptr);

  ASSERT_EQ(block.avail(), 1);
  ASSERT_EQ(block.size(), 1);
  ASSERT_FALSE(block.empty());
  ASSERT_FALSE(block.full());

  // Second allocation is likewise good, but the block is full.
  auto slot2 = block.allocate();
  ASSERT_EQ(slot2.size(), 42);
  ASSERT_NE(slot2.data(), nullptr);

  ASSERT_EQ(block.avail(), 0);
  ASSERT_EQ(block.size(), 2);
  ASSERT_FALSE(block.empty());
  ASSERT_TRUE(block.full());

  // Third allocation should fail.
  auto slot3 = block.allocate();
  ASSERT_EQ(slot3.size(), 0);
  ASSERT_EQ(slot3.data(), nullptr);
}


TEST(MemoryPacketBlock, deallocation)
{
  using namespace channeler::memory;
  packet_block<2> block{42};

  EXPECT_EQ(block.avail(), 2);
  EXPECT_EQ(block.size(), 0);
  EXPECT_TRUE(block.empty());
  EXPECT_FALSE(block.full());

  // First allocation is good.
  auto slot = block.allocate();
  ASSERT_EQ(slot.size(), 42);
  ASSERT_NE(slot.data(), nullptr);

  ASSERT_EQ(block.avail(), 1);
  ASSERT_EQ(block.size(), 1);
  ASSERT_FALSE(block.empty());
  ASSERT_FALSE(block.full());

  // Second allocation is likewise good, but the block is full.
  auto slot2 = block.allocate();
  ASSERT_EQ(slot2.size(), 42);
  ASSERT_NE(slot2.data(), nullptr);

  ASSERT_EQ(block.avail(), 0);
  ASSERT_EQ(block.size(), 2);
  ASSERT_FALSE(block.empty());
  ASSERT_TRUE(block.full());

  // Now deallocate the first slot
  block.free(slot);
  ASSERT_EQ(slot.size(), 0);
  ASSERT_EQ(slot.data(), nullptr);

  ASSERT_EQ(block.avail(), 1);
  ASSERT_EQ(block.size(), 1);
  ASSERT_FALSE(block.empty());
  ASSERT_FALSE(block.full());

  // This means a third allocation should succeed.
  auto slot3 = block.allocate();
  ASSERT_EQ(slot3.size(), 42);
  ASSERT_NE(slot3.data(), nullptr);

  ASSERT_EQ(block.avail(), 0);
  ASSERT_EQ(block.size(), 2);
  ASSERT_FALSE(block.empty());
  ASSERT_TRUE(block.full());
}
