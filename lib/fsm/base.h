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
#ifndef CHANNELER_FSM_BASE_H
#define CHANNELER_FSM_BASE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include "../pipe/event.h"
#include "../pipe/action.h"

namespace channeler::fsm {

/**
 * This file defines a base class for finite state machines that handle
 * parts of the channeler sub-protocols.
 *
 * The general interface is similar to the filter interface (see ../pipe),
 * in that FSMs consume events, whose type is defined in the pipe submodule.
 *
 * Unlike filters, each FSM produces actions (to be passed back down the
 * input filter pipe) as well as events (to be passed to the output filter
 * pipe). A FSM also does not consume the event it is being passed, but returns
 * whether or not it processed the event.
 *
 * As a result of this, actions and output events are appended to modifiable
 * list parameters, and only the processing status is returned.
 *
 * Also unlike filters, there is no statically compiled pipeline to FSMs.
 * Rather, each instance may or may not process an event and produce some
 * results. We therefore need an abstract base class.
 *
 * Finally, the input event types are fairly limited. They are invoked at the
 * end of the input filter pipe, where events are ET_MESSAGE (or in future,
 * timeouts TODO). This is checked at run-time however.
 * https://gitlab.com/interpeer/channeler/-/issues/10
 */
struct fsm_base
{
  /**
   * Process function, see above.
   */
  virtual bool process(::channeler::pipe::event * to_process,
      ::channeler::pipe::action_list_type & result_actions,
      ::channeler::pipe::event_list_type & output_events) = 0;

  virtual ~fsm_base() = default;
};

} // namespace channeler::fsm

#endif // guard
