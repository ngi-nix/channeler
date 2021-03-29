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
#ifndef CHANNELER_MEMORY_PACKET_BUFFER_H
#define CHANNELER_MEMORY_PACKET_BUFFER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <list>

#include "packet_pool.h"
#include "../lock_policy.h"

#include <channeler/packet.h>

namespace channeler::memory {

/**
 * When packets are received (or sent), they must be temporarily buffered
 * before e.g. the application can consume them, or they are no longer required
 * for resending.
 *
 * Buffering is highly dependent on the circumstances, i.e. the capabilities of
 * the channel for which packets are buffered. As such, a single buffer
 * implementation is not sufficient.
 *
 * Because of this, we manage the buffer memory in the packet_pool class -
 * in fact, multiple buffers can share a single pool. What buffer implementations
 * instead manage is how packets are added to and removed from the buffer.
 *
 * We need to deal with ordering, temporary loss (i.e. resend), and permanent
 * loss of packets. While handling loss via triggering resends or closing
 * channels is definitely not part of the scope of buffers, the buffer needs
 * to be able to report any perceived gaps in packet sequences.
 *
 * We implement this in the buffer, because it relates to the sequence in
 * which packets are extracted from the buffer.
 *
 * This level of functionality requires that the buffer class is sufficiently
 * aware of the packet sequence to perform these operations. We therfore
 * define an entry in the buffer as having a sequence number and a memory
 * slot. An understanding of other packet data is not required here.
 *
 * TODO
 * - only implement FIFO for now
 * - implement when filter pipe is reasonably implemented.
 * https://gitlab.com/interpeer/channeler/-/issues/14
 */
template <
  std::size_t POOL_BLOCK_SIZE,
  typename lock_policyT = null_lock_policy
>
class packet_buffer
{
public:
  using pool_type = packet_pool<POOL_BLOCK_SIZE, lock_policyT>;
  using slot_type = typename pool_type::slot;

  struct buffer_entry
  {
    packet_wrapper  packet;
    slot_type       data;
  };

  // TODO also take pool?
  inline packet_buffer(std::size_t packet_size, lock_policyT * lock = nullptr)
    : m_packet_size{packet_size}
    , m_lock{lock}
  {
  }


  inline error_t push(packet_wrapper const & packet, slot_type slot)
  {
    buffer_entry entry{packet, slot};
    m_buffer.push_back(entry);
    return ERR_SUCCESS;
  }

  // TODO pop for reading
  inline bool empty() const
  {
    return m_buffer.empty();
  }

private:

  // TODO this is an unbounded FIFO. A real implementation must provide
  // a bounded FIFO, implement loss behaviour on pop(), etc.
  using list_t = std::list<buffer_entry>;
  list_t          m_buffer;

  // Packet size
  std::size_t     m_packet_size;

  // Lock policy
  lock_policyT *  m_lock;
};


} // namespace channeler::memory

#endif // guard
