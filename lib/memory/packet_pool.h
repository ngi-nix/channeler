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
#ifndef CHANNELER_MEMORY_PACKET_POOL_H
#define CHANNELER_MEMORY_PACKET_POOL_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <set>

#include "packet_block.h"
#include "../lock_policy.h"

namespace channeler::memory {


/**
 * The packet_pool class manages a dynamic list of packet_block instances.
 *
 * Where packet_block is deliberately primitive, packet_pool keeps some
 * bookkeeping data, and can be used concurrently with an appropriate lock
 * strategy. While locking access to a central component such as a memory pool
 * is not ideal for performance, it is also somewhat necessary.
 *
 * We use a fairly coarse-grained approach to locking.
 *
 * Allocation returns holder objects that automatically deallocate the packet
 * when going out of scope. Holders internally use shared_ptr, so can be copied
 * relatively freely and inexpensively.
 *
 * One specific characeristic of this pool implementation is that it keeps
 * empty blocks around for future allocations to use. You must manually prune
 * empty blocks.
 */
template <
  std::size_t BLOCK_SIZE, // In packets
  typename lock_policyT = null_lock_policy
>
class packet_pool
{
private:
  using block_type = packet_block<BLOCK_SIZE>;

  // We maintain a list of blocks, with a freelist. This freelist points at
  // blocks with free elements.
  struct block_entry
  {
    inline block_entry(std::size_t packet_size)
      : block{packet_size}
    {}

    block_entry * next = nullptr;
    block_entry * next_free = nullptr;
    block_type    block;
  };

  // We wrap slot_impl into a struct within a shared_ptr so we don't have to
  // deal with lifetime management ourselves.
  struct slot_impl
  {
    packet_pool &             pool;
    block_entry *             block;
    typename block_type::slot block_slot;

    inline ~slot_impl()
    {
      // Since slot_impl is only ever passed around as a shared pointer, the
      // destructor is only called when the refcount goes to zero. We can
      // therefore free the slot here.
      guard g{pool.m_lock};
      pool.free_internal(block, block_slot);
    }
  };

public:
  class slot
  {
  public:
    inline byte * data()
    {
      if (!m_impl) {
        return nullptr;
      }
      return m_impl->block_slot.data();
    }

    inline byte const * data() const
    {
      if (!m_impl) {
        return nullptr;
      }
      return m_impl->block_slot.data();
    }

    inline std::size_t size() const
    {
      if (!m_impl) {
        return 0;
      }
      return m_impl->block_slot.size();
    }

    inline std::size_t use_count() const
    {
      return m_impl.use_count();
    }

    inline bool operator==(slot const & other) const
    {
      return m_impl == other.m_impl;
    }

  private:
    friend class packet_pool;

    inline slot() = default;

    inline slot(packet_pool & pool, block_entry * block,
        typename block_type::slot const & bs)
      : m_impl{new slot_impl{pool, block, bs}}
    {
    }

    std::shared_ptr<slot_impl> m_impl;
  };

  // Constructor
  inline packet_pool(std::size_t packet_size, lock_policyT * lock = nullptr)
    : m_packet_size{packet_size}
    , m_lock{lock}
  {
  }

  inline ~packet_pool()
  {
    guard g{m_lock};

    // Just delete all blocks
    block_entry * cur = m_blocks;
    while (cur) {
      block_entry * next = cur->next;
      delete cur;
      cur = next;
    }

    m_blocks = m_freelist = nullptr;
  }

  // Size accessors
  inline std::size_t size() const
  {
    guard g{m_lock};
    if (m_blocks == nullptr) {
      return 0;
    }

    // Ok, we have to look at all blocks' sizes
    std::size_t count = 0;
    block_entry * cur = m_blocks;
    while (cur) {
      count += cur->block.size();
      cur = cur->next;
    }

    return count;
  }

  inline std::size_t capacity() const
  {
    guard g{m_lock};
    if (m_blocks == nullptr) {
      return 0;
    }

    // Ok, we have to look at all blocks' sizes
    std::size_t count = 0;
    block_entry * cur = m_blocks;
    while (cur) {
      count += cur->block.capacity();
      cur = cur->next;
    }

    return count;
  }

  inline bool empty() const
  {
    guard g{m_lock};

    // If we have no blocks, the pool is empty
    if (m_blocks == nullptr) {
      return true;
    }

    // Otherwise, we have to check the freelist - if any of the free blocks
    // report slots in use, we're not empty.
    block_entry * cur = m_freelist;
    while (cur) {
      if (!cur->block.empty()) {
        return false;
      }
      cur = cur->next_free;
    }

    // All blocks with free slots, checked and all of them contained something.
    return true;
  }

  inline slot allocate()
  {
    guard g{m_lock};

    // Check if there is a block with free slots - it should be in the freelist
    block_entry * block = m_freelist;
    if (!block) {
      block = allocate_block();
    }

    // We now need a slot from the block.
    auto s = block->block.allocate();
    if (!s.size()) {
      // We have nothing. Note that this should never be reached, because it
      // would imply a newly allocated block has no slots left.
      return {};
    }

    // If the block is now full, we need to take it out of the free list.
    if (block->block.full()) {
      m_freelist = m_freelist->next_free;
    }

    return {*this, block, s};
  }

  inline void free(slot & s)
  {
    if (!s.m_impl) {
      return;
    }

    // We will do one runtime check, which is whether the slot actually belongs
    // to this block.
    if (&(s.m_impl->pool) != this) {
      throw exception{ERR_INVALID_REFERENCE,
        "Memory slot does not belong to the current pool."};
    }

    guard g{m_lock};

    free_internal(s.m_impl->block, s.m_impl->block_slot);

    // We *must* empty the slot in order to avoid double frees.
    s.m_impl = nullptr;
  }

  inline void prune()
  {
    guard g{m_lock};

    std::set<block_entry *> pruned;

    // Pruning empty blocks is a potentially expensive operation. We must
    // remove the entry from the block list, and from the free list. We know
    // that the free list is always going to be smaller or equal to the block
    // list, so pruning from the blocklist first is likely going to be a
    // bit faster.
    block_entry * cur = m_blocks;
    block_entry * prev = nullptr;
    while (cur) {
      if (cur->block.empty()) {
        // Take out of block list by jumping over it
        if (prev) {
          prev->next = cur->next;
        }
        else {
          m_blocks = cur->next;
        }

        // Add to prune set
        pruned.insert(cur);
      }

      prev = cur;
      cur = cur->next;
    }

    // We have to do the same with the free list, with the exception that we
    // already know to remove the entry. On the other hand, we have to do this
    // one for every item in the pruned set.
    cur = m_freelist;
    prev = nullptr;
    while (cur) {
      auto f = pruned.find(cur);
      if (f != pruned.end()) {
        if (prev) {
          prev->next_free = cur->next_free;
        }
        else {
          m_freelist = cur->next_free;
        }
      }

      prev = cur;
      cur = cur->next_free;
    }

    // Finally, we can deallocate all entries from the pruned set
    for (auto prune : pruned) {
      delete prune;
    }
  }

private:

  inline block_entry * allocate_block()
  {
    block_entry * n = new block_entry{m_packet_size};

    // Insert into blocks list.
    n->next = m_blocks;
    m_blocks = n;

    // Insert into free list
    n->next_free = m_freelist;
    m_freelist = n;

    return n;
  }

  inline void free_internal(block_entry * entry, typename block_type::slot & block_slot)
  {
    // First free the block slot from the block. This should be trivial.
    entry->block.free(block_slot);

    // If the block is now empty, we'll have to add it to the free list.
    // But we have to be careful first to ensure it's not already there.
    if (entry->block.empty()) {
      block_entry * cur = m_freelist;
      bool found = false;
      while (cur) {
        if (cur == entry) {
          found = true;
          break;
        }
        cur = cur->next_free;
      }

      if (!found) {
        entry->next_free = m_freelist;
        m_freelist = entry;
      }
    }

  }

  // Packet size
  std::size_t     m_packet_size;

  // We maintain first a linked list of blocks
  block_entry *   m_blocks = nullptr;

  // Second, we maintain a pointer to the next block with free slots
  block_entry *   m_freelist = nullptr;

  // Lock policy
  lock_policyT *  m_lock;
};

} // namespace channeler::memory

#endif // guard
