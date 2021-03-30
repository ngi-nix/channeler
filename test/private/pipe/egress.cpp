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

#include "../lib/pipe/egress.h"
#include "../lib/context.h"

#include <gtest/gtest.h>

namespace {

constexpr static std::size_t PACKET_SIZE = 300;

using ctx_t = channeler::default_context<
  int,
  3
>;

using egress_t = channeler::pipe::default_egress<ctx_t>;


} // anonymous namespace


TEST(PipeEgress, create)
{
  using namespace channeler::pipe;

  ctx_t ctx{
    200,
    [](ctx_t::timeouts_type::duration d) { return d; },
    []() -> std::vector<std::byte> { return {}; }
  };

  typename egress_t::channel_set chs{PACKET_SIZE};

  std::unique_ptr<event> caught;
  egress_t egress{
    [&caught](std::unique_ptr<event> ev) -> action_list_type
    {
      caught = std::move(ev);
      return {};
    },
    chs
  };
}
