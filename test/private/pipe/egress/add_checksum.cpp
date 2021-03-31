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

#include "../lib/pipe/egress/add_checksum.h"

#include <gtest/gtest.h>

#include "../../../packets.h"

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
constexpr std::size_t PACKET_SIZE = 200;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;

struct next
{
  using input_event = channeler::pipe::packet_out_event<POOL_BLOCK_SIZE>;

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    return {};
  }
  std::unique_ptr<channeler::pipe::event> m_event;
};


using filter_t = channeler::pipe::add_checksum_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event
>;

} // anonymous namespace



TEST(PipeIngressAddChecksumFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};
  next n;
  filter_t filter{&n};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}


TEST(PipeIngressAddChecksumFilter, checksum)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};
  next n;
  filter_t filter{&n};

  // Create packet
  auto slot = pool.allocate();
  memcpy(slot.data(), test::packet_default_channel,
      test::packet_default_channel_size);
  auto packet = channeler::packet_wrapper(slot.data(), slot.size(), true);

  // Mess up checksum
  auto sum = packet.checksum();
  packet.checksum() = 0;
  ASSERT_FALSE(packet.has_valid_checksum());

  // Pass through filter
  auto ev = std::make_unique<packet_out_event<POOL_BLOCK_SIZE>>(
      std::move(slot),
      std::move(packet)
  );

  auto ret = filter.consume(std::move(ev));

  ASSERT_EQ(0, ret.size());

  // No need to actually test packet header parsing - that's been tested
  // elsewhere. This tests that the filter passes on things well.
  ASSERT_EQ(n.m_event->type, ET_PACKET_OUT);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());

  ASSERT_EQ(sum, ptr->packet.checksum());
  ASSERT_TRUE(ptr->packet.has_valid_checksum());
}
