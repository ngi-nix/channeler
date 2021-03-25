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
#ifndef CHANNELER_FSM_DATA_H
#define CHANNELER_FSM_DATA_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>
#include <iostream> // FIXME

#include "base.h"

#include <channeler/message.h>

#include "../channels.h"
#include "../channel_data.h"
#include "../support/timeouts.h"

namespace channeler::fsm {

/**
 * Implement the channel initiator part.
 *
 * Implement the "data transport" state machine.
 *
 * In principle, this is actually so simple that calling it a FSM is wrong and
 * weird. In practice, though, it fits the code structure well to do so.
 *
 * This "state machine" does two jobs:
 *
 * - It translates data the user writes to a channel into outgoing data
 *   messages.
 * - It translates incoming data messages on a channel into a user
 *   notification that data is available for reading.
 *
 * That's pretty much it.
 *
 * Of course, it can only do that if the referenced channels are known. So if
 * they are not known, it may also generate some kind of errors. This last part
 * is complicated by the fact that a channel may be known but pending.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT
>
struct fsm_data
  : public fsm_base
{
  using channel_set = ::channeler::channels<channelT>;

  using message_event_type = ::channeler::pipe::message_event<addressT, POOL_BLOCK_SIZE, channelT>;
  using data_written_event_type = ::channeler::pipe::user_data_written_event;
  using data_to_read_event_type = ::channeler::pipe::user_data_to_read_event<POOL_BLOCK_SIZE>;


  /**
   * Need to keep a reference to a channel_set as well as the function for
   * producing the cookie secret.
   */
  inline fsm_data(channel_set & channels)
    : m_channels{channels}
  {
  }



  /**
   * Process function, see above.
   */
  virtual bool process(::channeler::pipe::event * to_process,
      ::channeler::pipe::action_list_type & result_actions,
      ::channeler::pipe::event_list_type & output_events)
  {
    namespace pipe = channeler::pipe;

    // Distinguish incoming event types
    switch (to_process->type) {
      case pipe::ET_MESSAGE:
        {
          auto msg_ev = reinterpret_cast<message_event_type *>(to_process);
          return handle_message(msg_ev, result_actions, output_events);
        }

      case pipe::ET_USER_DATA_WRITTEN:
        {
          auto data_ev = reinterpret_cast<data_written_event_type *>(to_process);
          return handle_user_data_written(data_ev, result_actions, output_events);
        }

      default:
        break;
    }

    // Otherwise we can't really do anything here.
    return false;
  }


  inline bool handle_message(message_event_type * event,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events)
  {
    if (event->message->type != MSG_DATA) {
      // We process only data messages.
      return false;
    }

    // The main difference in how we handle things here is by the channel
    // state. We can only process messages on established channels.
    // We know this from the channel_assign filter by looking at the
    // channel pointer we're being passed.
    if (!m_channels.has_established_channel(event->packet.channel())) {
      // XXX: This should never happen. It's a programming bug, due to how this
      //      class *should* be used. But things that should never happen have
      //      a way of happening anyway, so... maybe we can return an action
      //      here if we do find an occurrence?
      return true;
    }

    // Since this is for a channel we know, we need to copy the data payload
    // into an event for the user to consume.
    auto result = std::make_unique<data_to_read_event_type>(
        event->packet.channel(),
        event->data,
        std::move(event->message)
    );
    output_events.push_back(std::move(result));

    return true;
  }


  inline bool handle_user_data_written(data_written_event_type * event,
      ::channeler::pipe::action_list_type & result_actions,
      ::channeler::pipe::event_list_type & output_events)
  {
    // If we don't know the given channel, then we must error out via an
    // action.
    if (!m_channels.has_channel(event->channel)) {
      result_actions.push_back(std::make_unique<channeler::pipe::error_action>(
            ERR_INVALID_CHANNELID));
      return true;
    }

    // If we have a channel, it doesn't matter if pending or established, we
    // expect the data to be buffered for output.
    auto ch = m_channels.get(event->channel);
    auto ret = ch->add_outgoing_data(event->data);
    if (ret < 0) {
      result_actions.push_back(std::make_unique<channeler::pipe::error_action>(
            ERR_WRITE));
    }

    // If the channel was not pending, we also need to produce an event.
    // The only payload that is necessary is the channel identifier, for
    // the pipe to start producing messages.
    if (m_channels.has_established_channel(event->channel)) {
      output_events.push_back(std::make_unique<channeler::pipe::user_data_to_send_event>(
            event->channel));
    }

    return true;
  }



  virtual ~fsm_data() = default;

private:

  channel_set &                     m_channels;
};

} // namespace channeler::fsm

#endif // guard
