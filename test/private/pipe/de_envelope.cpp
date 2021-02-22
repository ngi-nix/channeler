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

#include "../lib/pipe/de_envelope.h"

#include <gtest/gtest.h>

namespace {

// For testing
using address_t = uint16_t;
constexpr std::size_t POOL_BLOCK_SIZE = 3;
constexpr std::size_t PACKET_SIZE = 42;

using pool_type = ::channeler::memory::packet_pool<POOL_BLOCK_SIZE>;

struct next
{
  struct input_event : public channeler::pipe::event
  {
    inline input_event(address_t src, address_t dst, channeler::public_header_fields f,
        pool_type::slot slot)
      : event{channeler::pipe::ET_PARSED_HEADER}
      , src_addr{src}
      , dst_addr{dst}
      , header{f}
      , data{slot}
    {
    }

    address_t src_addr;
    address_t dst_addr;
    channeler::public_header_fields header;
    pool_type::slot data;
  };

  inline channeler::pipe::action_list_type consume(std::unique_ptr<channeler::pipe::event> event)
  {
    m_event = std::move(event);
    return {};
  }
  std::unique_ptr<channeler::pipe::event> m_event;
};

using filter_t = channeler::pipe::de_envelope_filter<
  address_t,
  POOL_BLOCK_SIZE,
  next,
  next::input_event
>;

} // anonymous namespace



TEST(DeEnvelopeFilter, throw_on_invalid_event)
{
  using namespace channeler::pipe;

  next n;
  filter_t filter{&n};

  // Create a default event; this should not be handled.
  auto ev = std::make_unique<event>();
  ASSERT_THROW(filter.consume(std::move(ev)), ::channeler::exception);

  // The filter should also throw on a null event
  ASSERT_THROW(filter.consume(nullptr), ::channeler::exception);
}



TEST(DeEnvelopeFilter, parse_data)
{
  using namespace channeler::pipe;

  pool_type pool{PACKET_SIZE};

  auto data = pool.allocate();

  next n;
  filter_t filter{&n};

  // No data added to event.
  auto ev = std::make_unique<filter_t::input_event>(123, 321, data);
  ASSERT_NO_THROW(filter.consume(std::move(ev)));

  // No need to actually test packet header parsing - that's been tested
  // elsewhere. This tests that the filter passes on things well.
  ASSERT_EQ(n.m_event->type, ET_PARSED_HEADER);
  next::input_event * ptr = reinterpret_cast<next::input_event *>(n.m_event.get());
  ASSERT_EQ(123, ptr->src_addr);
  ASSERT_EQ(321, ptr->dst_addr);
}
