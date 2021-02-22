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
#ifndef CHANNELER_MEMORY_PACKET_BLOCK_H
#define CHANNELER_MEMORY_PACKET_BLOCK_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

namespace channeler::memory {

/**
 * The packet block, as the name implies, manages memory for a block of
 * packets.
 *
 * It is different from a general purpose memory block in that we do not
 * care about alignment of memory on anything other than byte boundaries,
 * because this is not a block of some type, but of byte arrays. We merely
 * require the size of each byte array, as well as the number of such arrays
 * to manage.
 *
 * The class is meant to be primitive and does not care about serializing
 * access to its functions.
 */
template <
  std::size_t CAPACITY
>
class packet_block
{
private:
  // Memory is managed in chunks; each chunk contains a header, which
  // encapsulates the free list, etc.
  struct chunk;
  struct chunk_header
  {
    chunk * next = nullptr;
  };

  // Chunks are simple structs themselves.
  struct chunk
  {
    chunk_header  header;
    std::byte *   data;
  };

public:
  // *************************************************************************
  // We allocate and deallocate in slots.
  struct slot
  {
    inline std::byte * data()
    {
      if (m_index == capacity()) {
        return nullptr;
      }
      return m_block.m_chunks[m_index].data;
    }

    inline std::size_t size() const
    {
      if (m_index == capacity()) {
        return 0;
      }
      return m_packet_size;
    }

  private:
    friend class packet_block;

    inline slot(packet_block & block, std::size_t index, std::size_t packet_size)
      : m_block{block}
      , m_index{index}
      , m_packet_size{packet_size}
    {
    }

    packet_block &    m_block;
    std::size_t       m_index;
    std::size_t const m_packet_size;
  };

  // *************************************************************************
  // Block implementation
  inline packet_block(std::size_t packet_size)
    : m_packet_size{packet_size}
    , m_data{new std::byte[m_packet_size * CAPACITY]}
    , m_chunks{new chunk[CAPACITY]}
  {
    // Allocate data and chunks.

    // We have to initialize the free list. This means a) setting the freelist
    // to point at the first chunk, and b) setting the header of each chunk but
    // the last to the next chunk.
    m_freelist = m_chunks;

    for (std::size_t i = 0 ; i < CAPACITY - 1 ; ++i) {
      m_chunks[i].header.next = &(m_chunks[i + 1]);
      m_chunks[i].data = m_data + (i * m_packet_size);
    }
    m_chunks[CAPACITY - 1].header.next = nullptr;
    m_chunks[CAPACITY - 1].data = m_data + ((CAPACITY - 1) * m_packet_size);
  }

  inline ~packet_block()
  {
    delete [] m_data;
    delete [] m_chunks;
  }


  // Metadata
  static constexpr inline std::size_t capacity()
  {
    return CAPACITY;
  }

  inline std::size_t packet_size() const
  {
    return m_packet_size;
  }

  inline std::size_t memory_size() const
  {
    return packet_size() * capacity();
  }


  // The size in STL containers is the number of allocated elements. We have
  // a free list, though, so it's easier to report availability.
  inline std::size_t avail() const
  {
    std::size_t counter = 0;
    chunk * cur = m_freelist;
    while (cur) {
      ++counter;
      cur = cur->header.next;
    }
    return counter;
  }

  inline std::size_t size() const
  {
    return capacity() - avail();
  }

  // Similarly, reporting empty() requires use of the free list, while
  // reporting full() is fast.
  inline bool empty() const
  {
    return avail() == capacity();
  }

  inline bool full() const
  {
    return m_freelist == nullptr;
  }


  // Slot allocation and deallocation
  inline slot allocate()
  {
    // Allocation is largely finding the right slot index. Since we have a
    // free list, that is simple enough.
    if (m_freelist == nullptr) {
      return {*this, capacity(), m_packet_size};
    }

    // Ok, we know m_freelist points at the first available slot.
    // We can find the index of the slot easily enough.
    std::size_t idx = m_freelist - m_chunks;

    // "Allocate" by changing the free list. If the next pointer is
    // nullptr, that means we just took the last chunk.
    chunk & cur = *m_freelist;
    m_freelist = cur.header.next;
    cur.header.next = nullptr; // Take out of the list entirely

    return {*this, idx, m_packet_size};
  }


  inline void free(slot & s)
  {
    // We will do one runtime check, which is whether the slot actually belongs
    // to this block.
    if (&(s.m_block) != this) {
      throw exception{ERR_INVALID_REFERENCE,
        "Memory slot does not belong to the current block."};
    }

    // Unused slots are detected by pointing at capacity
    if (s.m_index == capacity()) {
      return;
    }

    // Re-adding the slot to the free list is pretty simple; all we need to
    // do is prepend it. If the freelist is currently nullptr or points to a
    // chunk is irrelevant, it works either way.
    m_chunks[s.m_index].header.next = m_freelist;
    m_freelist = &(m_chunks[s.m_index]);

    // Finally, mark the slot variable as unused.
    s.m_index = capacity();
  }


private:
  // Keep a notion of the packet size around
  std::size_t const m_packet_size;

  // Chunk headers and data.
  std::byte *       m_data = nullptr;
  chunk *           m_chunks = nullptr;

  // We also need a free list is the head of the current free list, as a pointer
  // to a chunk header. If this is NULL, there are no free elements.
  chunk *           m_freelist = nullptr;
};


} // namespace channeler::memory

#endif // guard
