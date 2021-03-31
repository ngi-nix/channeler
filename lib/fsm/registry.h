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
#ifndef CHANNELER_FSM_REGISTRY_H
#define CHANNELER_FSM_REGISTRY_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include "base.h"

namespace channeler::fsm {

/**
 * A run-time registry of FSMs, to simplify state handling across multiple
 * FSM implementations.
 */
class registry
{
public:

  /**
   * Instanciate and add FSM. There is no removal of FSMs, as it is expected
   * that the number and composition of FSMs does not change during the run-time
   * of the protocol. The registry only matters in compositing; for this reason,
   * we also subsume the construction into the add() function.
   */
  template <
    typename fsmT,
    typename... ctor_argsT
  >
  inline void add(ctor_argsT &&... ctor_args)
  {
    static_assert(std::is_base_of<fsm_base, fsmT>::value,
        "fsmT must be a descendant of fsm_base");

    m_fsms.insert(std::move(
          std::make_unique<fsmT>(std::forward<ctor_argsT>(ctor_args)...)
    ));
  }

  template <
    typename fsmT
  >
  inline void add_move(std::unique_ptr<fsmT> fsm)
  {
    static_assert(std::is_base_of<fsm_base, fsmT>::value,
        "fsmT must be a descendant of fsm_base");
    m_fsms.insert(std::move(fsm));
  }


  /**
   * Process the input event across all FSMs. Returns true if at least one
   * registered FSM processed the input event.
   */
  inline bool process(::channeler::pipe::event * to_process,
      ::channeler::pipe::action_list_type & result_actions,
      ::channeler::pipe::event_list_type & output_events)
  {
    if (!to_process) {
      return false;
    }

    bool processed = false;

    for (auto & fsm : m_fsms) {
      if (fsm->process(to_process, result_actions, output_events)) {
        processed = true;
      }
    }

    return processed;
  }

private:
  using fsm_ptr = std::unique_ptr<fsm_base>;
  using fsm_set = std::set<fsm_ptr>;

  fsm_set     m_fsms;
};


} // namespace channeler::fsm

#endif // guard
