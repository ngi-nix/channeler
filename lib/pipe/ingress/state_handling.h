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
#ifndef CHANNELER_PIPE_INGRESS_STATE_HANDLING_H
#define CHANNELER_PIPE_INGRESS_STATE_HANDLING_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>
#include <map>

#include "../../lock_policy.h"
#include "../event.h"
#include "../action.h"
#include "../event_as.h"


namespace channeler::pipe {


/**
 * Handles the state machines for messages.
 *
 * This filter is the *end* of the ingress pipe, and the *beginning* of the
 * egress pipe -- the latter albeit only for messages produced by the FSMs
 * it encapsulates.
 *
 * At the same time, the FSMs may produce notifications to the API user, such
 * as of errors or data being available.
 *
 * The filter therefore requires as a paremter a mapping of event categories
 * to recipient functions. At this point, it seems unlikely that individual
 * event types need to be routed differently.
 *
 * The function prototype follows that of the filter's consume function,
 * in that it can produce actions. It cannot be injecting more events
 * into *this* pipe.
 */
using event_route_function = std::function<action_list_type (std::unique_ptr<event>)>;
using event_route_map = std::map<event_category, event_route_function>;

template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT,
  typename fsm_registryT
>
struct state_handling_filter
{
  using input_event = message_event<addressT, POOL_BLOCK_SIZE, channelT>;

  inline state_handling_filter(fsm_registryT & registry, event_route_map & route_map)
    : m_registry{registry}
    , m_event_route_map{route_map}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    auto in = event_as<input_event>("ingress:state_handling", ev.get(), ET_MESSAGE);

    action_list_type actions;
    event_list_type events;

    auto processed = m_registry.process(in, actions, events);
    if (!processed) {
      LIBLOG_WARN("Mmessage was not processed by registry: " << in->message->type);
    }

    for (auto & shared_ev : events) {
      auto iter = m_event_route_map.find(shared_ev->category);
      if (iter == m_event_route_map.end()) {
        LIBLOG_ERROR("Do not know how to dispatch result event of category: "
            << shared_ev->category);
        continue;
      }

      auto acts = iter->second(std::move(shared_ev));
      actions.merge(acts);
    }

    LIBLOG_DEBUG("Returning actions: " << actions.size());
    return actions;
  }


  fsm_registryT &   m_registry;
  event_route_map & m_event_route_map;
};


} // namespace channeler::pipe

#endif // guard
