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
#ifndef CHANNELER_FSM_CHANNEL_INITIATOR_H
#define CHANNELER_FSM_CHANNEL_INITIATOR_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>

#include "base.h"

#include <channeler/message.h>

#include "../channels.h"
#include "../channel_data.h"
#include "../support/timeouts.h"

namespace channeler::fsm {

/**
 * Implement the channel initiator part.
 *
 * Channel initiators have actual states. The state is managed per channel,
 * and can take one of the following values:
 *
 * - Start state - no channel initiation attempt has been made yet.
 * - Pending - a MSG_CHANNEL_NEW has been sent, but no response has
 *   been received.
 * - Established - a MSG_CHANNEL_ACKNOWLEDGE or correspong MSG_CHANNEL_COOKIE
 *   has been received, and a MSG_CHANNEL_FINALIZE/_COOKIE has been sent in
 *   return.
 * - The error states are reached respectively if channel establishment timed
 *   out or aborted due to an error.
 *
 * There is some complication here in that the start state applies globally;
 * the other states apply to per-channel FSMs, which technically start in the
 * CHR_PENDING state. The state machines are correspondingly *very* simple, so
 * we'll skip even something as lightweight as microfsm here.
 *
 * In addition to processing message events, the state machine can also process
 * timeout events. In particular, the Initiating state can time out.
 *
 * The only other event being handled is the explicit initialization event that
 * is supplied via user interaction.
 */
constexpr uint16_t CHANNEL_NEW_TIMEOUT_TAG{0xc411};
constexpr uint16_t CHANNEL_TIMEOUT_TAG{0x114c};
using timeout_unit_type = uint64_t;
constexpr timeout_unit_type DEFAULT_CHANNEL_NEW_TIMEOUT{200 * 1000}; // 200 msec
constexpr timeout_unit_type DEFAULT_CHANNEL_TIMEOUT{60 * 1000 * 1000}; // 1 min

template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT,
  // TODO the timeouts should probably be run-time configurable
  //    https://gitlab.com/interpeer/channeler/-/issues/11
  timeout_unit_type CHANNEL_NEW_TIMEOUT = DEFAULT_CHANNEL_NEW_TIMEOUT,
  timeout_unit_type CHANNEL_TIMEOUT = DEFAULT_CHANNEL_TIMEOUT
>
struct fsm_channel_initiator
  : public fsm_base
{
  using channel_set = ::channeler::channels<channelT>;
  using secret_type = std::vector<std::byte>;
  using secret_generator = std::function<secret_type ()>;

  using new_channel_event_type = ::channeler::pipe::new_channel_event;
  using message_event_type = ::channeler::pipe::message_event<addressT, POOL_BLOCK_SIZE, channelT>;
  using timeout_event_type = ::channeler::pipe::timeout_event<
    ::channeler::support::timeout_scoped_tag_type
  >;


  /**
   * Need to keep a reference to a channel_set as well as the function for
   * producing the cookie secret.
   */
  inline fsm_channel_initiator(
      ::channeler::support::timeouts & timeouts,
      channel_set & channels, secret_generator generator)
    : m_timeouts{timeouts}
    , m_channels{channels}
    , m_secret_generator{generator}
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

    // We handle three types of events here, two of which get handled per-channel.
    switch (to_process->type) {
      case pipe::ET_NEW_CHANNEL:
        {
          auto new_ev = reinterpret_cast<new_channel_event_type *>(to_process);
          return initiate_new_channel(new_ev->sender, new_ev->recipient,
              result_actions, output_events);
        }

      case pipe::ET_MESSAGE:
        {
          auto msg_ev = reinterpret_cast<message_event_type *>(to_process);
          return handle_message(msg_ev, result_actions, output_events);
        }

      case pipe::ET_TIMEOUT:
        {
          auto to_ev = reinterpret_cast<timeout_event_type *>(to_process);
          return handle_timeout(to_ev, result_actions, output_events);
        }

      default:
        break;
    }

    // Otherwise we can't really do anything here.
    return false;
  }


  inline bool initiate_new_channel(
      ::channeler::peerid const & sender, // ourselves!
      ::channeler::peerid const & recipient,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    // Create pending channel in the channel set. This also initialises a
    // pending FSM, if the channel data holds one (it should!)
    auto id = m_channels.new_pending_channel();

    // Create a cookie
    auto secret = m_secret_generator();
    auto cookie1 = create_cookie_initiator(secret.data(), secret.size(),
        sender, recipient,
        id);

    // Create and return MSG_CHANNEL_NEW in output_events
    auto init = std::make_unique<message_channel_new>(id, cookie1);
    auto ev = std::make_shared<channeler::pipe::message_out_event>(
          sender, recipient,
          DEFAULT_CHANNELID,
          std::move(init)
        );
    output_events.push_back(ev);

    // Use timout provider to set a timeout with the context being a tuple
    // of a channel tag and the initiator part.
    m_timeouts.add({CHANNEL_NEW_TIMEOUT_TAG, id},
        ::channeler::support::timeouts::duration{CHANNEL_NEW_TIMEOUT});

    return true;
  }



  inline bool handle_message(message_event_type * event,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    // Check that message type is MSG_CHANNEL_ACKNOWLEDGE or abort.
    if (event->message->type != MSG_CHANNEL_ACKNOWLEDGE) {
      return false;
    }
    auto msg = reinterpret_cast<message_channel_acknowledge const *>(event->message.get());

    // Pick channel identifier from message, grab channel data for it. 
    auto channel = m_channels.get(msg->id);
    if (!channel) {
      // We cannot acknowledge a channel we know nothing about.

      return false;
    }

    // Channel must be in pending state.
    if (!m_channels.has_pending_channel(msg->id)) {
      // This is an acknowledgement for a channel that's already established.
      // This is unlikely with a well-behaving responder, so we should consider
      // this message as one we should not process.
      // XXX We may want to consider blocking this responder, but that has usage
      //     implications.
      return false;
    }

    // At this point, we need to verify that the cookie1 sent in the message
    // is valid. If so, we can move the channel into an established state.
    // If not, we have to abort.
    auto secret = m_secret_generator();
    auto cookie1 = create_cookie_initiator(secret.data(), secret.size(),
        event->packet.recipient(), // ourselves!
        event->packet.sender(), // responder
        msg->id.initiator);

    if (cookie1 != msg->cookie1) {
      // We cannot verify that the message is in response to one of our
      // requests. This may be because the cookie secret generator changed
      // the secret value. But that's not so easy to verify, so we'll
      // abort here.
      // TODO: since we're the initiator, we can safely store the cookie we
      //       sent in the channel data. It will take up some space, but it's
      //       not as if a well-behaving initiator will create tons of
      //       channels. Let's do that in the near future instead, then
      //       we don't have to recreate the cookie above.
      //       https://gitlab.com/interpeer/channeler/-/issues/12
      m_channels.remove(msg->id);
      return true;
    }

    // We'll upgrade the channel to full in the channel set.
    auto res = m_channels.make_full(msg->id);
    if (res != ERR_SUCCESS) {
      // This should really not happen at this point. But if it does, we'll
      // have to remove the channel and abort. Something in our channel
      // bookkeeping went very much wrong.
      m_channels.remove(msg->id);
      return true;
    }

    // At this point, we have an acknowledgement. From our point of view,
    // the channel is established. We'll just send a MSG_CHANNEL_FINALIZE or
    // MSG_CHANNEL_COOKIE to tell this to the other side.
    // However, if either of these messages were to get lost, the responder
    // may not receive data. It's now possible for the channel itself to
    // timeout. We have to cancel the CHANNEL_NEW_TIMEOUT, but start a
    // (much longer) CHANNEL_TIEMOUT.
    m_timeouts.remove({CHANNEL_NEW_TIMEOUT_TAG, msg->id.initiator});
    m_timeouts.add({CHANNEL_TIMEOUT_TAG, msg->id.initiator},
        ::channeler::support::timeouts::duration{CHANNEL_TIMEOUT});

    // Construct the finalize or cookie messages, respectively.
    if (channel->has_outgoing_data_pending()) {
      // TODO MSG_CHANNEL_COOKIE
      //      https://gitlab.com/interpeer/channeler/-/issues/13
    }
    else {
      // MSG_CHANNEL_FINALIZE
      auto response = std::make_unique<message_channel_finalize>(msg->id, msg->cookie2,
          capabilities_t{}); // TODO capabilities not used yet.
                             // https://gitlab.com/interpeer/channeler/-/issues/14
      auto ev = std::make_shared<channeler::pipe::message_out_event>(
            event->packet.recipient().copy(), event->packet.sender().copy(),
            DEFAULT_CHANNELID,
            std::move(response)
          );
      output_events.push_back(ev);
    }

    return true;
  }


  inline bool handle_timeout(timeout_event_type * event,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    // The first thing to check is the tag of the timeout.
    if (event->context.scope != CHANNEL_NEW_TIMEOUT_TAG
        && event->context.scope != CHANNEL_TIMEOUT_TAG)
    {
      // Not a timeout for us.
      return false;
    }

    // The tag itself is our initiator part of a channel identifier.
    channelid::half_type id = event->context.tag;

    // If we have a timeout - either timeout - with a channel id we know,
    // we'll have to remove it.
    if (m_channels.has_channel(id)) {
      // TODO add retries with exponential backoff up to a configurable
      //      limit
      //      https://gitlab.com/interpeer/channeler/-/issues/15
      m_channels.remove(id);
      return true;
    }

    // We didn't actually handle the timeout here.
    return false;
  }

  virtual ~fsm_channel_initiator() = default;

private:

  ::channeler::support::timeouts &  m_timeouts;
  channel_set &                     m_channels;
  secret_generator                  m_secret_generator;
};

} // namespace channeler::fsm

#endif // guard
