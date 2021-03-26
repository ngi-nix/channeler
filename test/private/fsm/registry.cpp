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

#include "../lib/fsm/base.h"

#include <gtest/gtest.h>

namespace {

struct foo {};

using namespace channeler::pipe;

struct test_fsm : public ::channeler::fsm::fsm_base
{
  virtual bool process(event * to_process [[maybe_unused]],
      action_list_type & result_actions,
      event_list_type & output_events)
  {
    // For the test case, we need to add one action and two
    // events.
    result_actions.push_back(std::make_unique<action>());

    output_events.push_back(std::make_unique<event>());
    output_events.push_back(std::make_unique<event>());

    return true;
  }

};


struct test_fsm_with_ctor : public ::channeler::fsm::fsm_base
{
  inline test_fsm_with_ctor(int unused [[maybe_unused]])
  {
  }

  virtual bool process(event * to_process [[maybe_unused]],
      action_list_type & result_actions [[maybe_unused]],
      event_list_type & output_events [[maybe_unused]])
  {
    return false;
  }
};

} // anonymous namespace


TEST(FSMRegistry, add)
{
  using namespace channeler::fsm;

  registry reg;

  // Fail at compile time
  // reg.add<foo>();

  // Succeed!
  ASSERT_NO_THROW(reg.add<test_fsm>());
}


TEST(FSMRegistry, add_with_ctor_args)
{
  using namespace channeler::fsm;

  registry reg;

  // Fail at compile time
  // reg.add<test_fsm_with_ctor>();

  // Succeed!
  ASSERT_NO_THROW(reg.add<test_fsm_with_ctor>(42));
}


TEST(FSMRegistry, with_ctx_add)
{
  using namespace channeler::fsm;

  registry<int> reg;

  // Fail at compile time
  // reg.add<foo>();

  // Succeed!
  ASSERT_NO_THROW(reg.add<test_fsm>());
}


TEST(FSMRegistry, with_ctx_add_with_ctor_args)
{
  using namespace channeler::fsm;

  registry<int> reg;

  // Fail at compile time
  // reg.add<test_fsm_with_ctor>();

  // Succeed!
  ASSERT_NO_THROW(reg.add<test_fsm_with_ctor>(42));
}
TEST(FSMRegistry, process_without_fsm)
{
  using namespace channeler::pipe;
  using namespace channeler::fsm;

  registry reg;

  // Without a FSM registered, process() must return false.
  event ev;
  action_list_type actions;
  event_list_type outputs;

  auto res = reg.process(&ev, actions, outputs);
  ASSERT_FALSE(res);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, outputs.size());
}


TEST(FSMRegistry, process_with_fsm)
{
  using namespace channeler::pipe;
  using namespace channeler::fsm;

  registry reg;
  EXPECT_NO_THROW(reg.add<test_fsm>());

  // With a FSM registered, process() must produce results
  event ev;
  action_list_type actions;
  event_list_type outputs;

  auto res = reg.process(&ev, actions, outputs);
  ASSERT_TRUE(res);
  ASSERT_EQ(1, actions.size());
  ASSERT_EQ(2, outputs.size());
}



TEST(FSMRegistry, process_with_fsm_empty_event)
{
  using namespace channeler::pipe;
  using namespace channeler::fsm;

  registry reg;
  EXPECT_NO_THROW(reg.add<test_fsm>());

  // With a FSM registered, but no input event, we don't get results.
  action_list_type actions;
  event_list_type outputs;

  auto res = reg.process(nullptr, actions, outputs);
  ASSERT_FALSE(res);
  ASSERT_EQ(0, actions.size());
  ASSERT_EQ(0, outputs.size());
}
