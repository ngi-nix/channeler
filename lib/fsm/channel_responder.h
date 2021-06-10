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
#ifndef CHANNELER_FSM_CHANNEL_RESPONDER_H
#define CHANNELER_FSM_CHANNEL_RESPONDER_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>

#include "base.h"

#include <channeler/message.h>

#include "../macros.h"
#include "../channels.h"
#include "../channel_data.h"

namespace channeler::fsm {

/**
 * Implement the channel responder part.
 *
 * Channel responders react to MSG_CHANNEL_NEW and MSG_CHANNEL_FINALIZE
 * messages, and crucially *do not* keep state. Instead, they use a cookie
 * mechanism for validating that the MSG_CHANNEL_FINALIZE was sent by a
 * peer that also sent a corresponding MSG_CHANNEL_NEW. This is to avoid
 * malicious initiators to use unnecessary resources on the responder side.
 *
 * However, two extra pieces of information influence the FSM "state". Purists
 * surely would consider the model below inaccurate, but it works well for our
 * purposes.
 *
 * a) If a MSG_CHANNEL_NEW arrives for a channel that is already established,
 *    we can treat this as a session refresh. In particular, if we respond with
 *    the same full channel identifier as is already in use, we don't really
 *    lose anything. We also don't reveal anything to a potential malicious
 *    initiator, because channel identifiers are scoped to peers.
 *
 *    Similarly, we can ignore any MSG_CHANNEL_FINALIZE that arrives for an
 *    already established channel.
 *
 *    - If a MSG_CHANNEL_FINALIZE arrives that sets different capability bits
 *      than already established for the channel, these are discarded. If in
 *      the future we want to update channel capabilities at run-time, those
 *      will be sent in a different message.
 *
 *    In either case, the FSM needs to be aware of which channels are already
 *    established.
 *
 * b) The other thing the FSM needs is some secret that can be used in the
 *    cookie generation. We're providing this in the form of a callback
 *    function.
 *
 *    The reason for providing a callback function is that it is feasible that
 *    the secret should be periodically modified, but that is outside of the
 *    scope of the FSM. A callback function allows for this to happen
 *    elsewhere.
 *
 *    Note that it's possible for the secret to change between two invocations
 *    during one handshake process. For example, if one secret was used during
 *    the generation of MSG_CHANNEL_ACKNOWLEDGE, but another was used when
 *    trying to verify MSG_CHANNEL_FINALIZE, the verification will fail. Rather
 *    than provide an error-prone process for avoiding this, we can just ignore
 *    this failure silently. The initiator will eventually consider the channel
 *    establishment process a failure, and may start over with a new
 *    MSG_CHANNEL_NEW.
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename channelT
>
struct fsm_channel_responder
  : public fsm_base
{
  using channel_set = ::channeler::channels<channelT>;
  using secret_type = std::vector<byte>;
  using secret_generator = std::function<secret_type ()>;

  using message_event_type = ::channeler::pipe::message_event<addressT, POOL_BLOCK_SIZE, channelT>;

  /**
   * Need to keep a reference to a channel_set as well as the function for
   * producing the cookie secret.
   */
  inline fsm_channel_responder(channel_set & channels, secret_generator generator)
    : m_channels{channels}
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

    // Ensure correct event and message type(s)
    if (to_process->type != pipe::ET_MESSAGE) {
      LIBLOG_WARN("Event type not handled by channel_responder: " << to_process->type);
      return false;
    }

    auto input = reinterpret_cast<message_event_type *>(to_process);
    auto msg_type = input->message->type;
    LIBLOG_DEBUG("Got message of type: " << msg_type);
    switch (msg_type) {
      case MSG_CHANNEL_NEW:
        return handle_new(reinterpret_cast<message_channel_new *>(input->message.get()),
            input->packet,
            result_actions, output_events);

      case MSG_CHANNEL_FINALIZE:
        return handle_finalize(reinterpret_cast<message_channel_finalize *>(input->message.get()),
            input->packet,
            result_actions, output_events);

      case MSG_CHANNEL_COOKIE:
        return handle_cookie(reinterpret_cast<message_channel_cookie *>(input->message.get()),
            input->packet,
            result_actions, output_events);

      default:
        LIBLOG_WARN("Message type not handled by channel_responder: " << msg_type);
        break;
    }

    // Any unhandled message
    return false;
  }


  inline bool handle_new(message_channel_new * msg,
      ::channeler::packet_wrapper const & packet,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    LIBLOG_DEBUG("MSG_CHANNEL_NEW(init["
        << std::hex << msg->initiator_part << "]/"
        << "cookie1[" << std::hex << msg->cookie1 << "])"
        << std::dec);
    // If we got a MSG_CHANNEL_NEW, we have one of three possible situations to
    // consider:
    // a) We already have a pending channel with this partial identifier. That
    //    means we have ourselves sent out a MSG_CHANNEL_NEW with this
    //    initiator part. That means we have some wires crossed and should
    //    resend the MSG_CHANNEL_NEW with a new initiator. But that is outside
    //    of the scope of this class, so we should drop the pending channel
    //    information instead. The channel_initiator class should retry with
    //    a new partial.
    // b) We have an established channel. That can only happen if we already
    //    received a MSG_CHANNEL_FINALIZE or equivalent MSG_CHANNEL_COOKIE. So
    //    the peer wants to re-establish the same channel, effectively. We can
    //    respond, but we need to re-use the same channel identifier.
    //    XXX It's probably wise to have a time stamp in the channel data so we
    //        can avoid being flooded here from a malicious peer.
    // c) We know nothing about this partial channel identifier. That's great,
    //    because we just need to send a cookie.
    if (m_channels.has_pending_channel(msg->initiator_part)) {
      m_channels.drop_pending_channel(msg->initiator_part);
      // TODO notify other channel_initiator via action?
      //      https://gitlab.com/interpeer/channeler/-/issues/16
      LIBLOG_ERROR("Received an init request for a pending channel; we'll abort.");
      return false;
    }

    // We send a cookie now either way.
    auto full_id = m_channels.get_established_id(msg->initiator_part);
    if (full_id == DEFAULT_CHANNELID) {
      // We *know* that m_channels does not know about the initiator here, so
      // any full identifier made from this partial is ok.
      full_id.initiator = msg->initiator_part;
      auto err = complete_channelid(full_id);
      if (err != ERR_SUCCESS) {
        // TODO report this somehow? I don't know what the exact right thing
        // to do here would be.
        // https://gitlab.com/interpeer/channeler/-/issues/17
        LIBLOG_ET("Could not complete the channel id", err);
        return false;
      }
    }

    // With the full identifier established, generate a responder cookie.
    auto secret = m_secret_generator();

    // Since we're responding to the MSG_CHANNEL_NEW, the packet sender is
    // the initiator, and the recipient (us) is the responder.
    auto cookie2 = create_cookie_responder(secret.data(), secret.size(),
        packet.sender(), packet.recipient(), full_id);

    // Construct message. If we have channel information for this channel,
    // and there is pending data for it (unlikely), we want to send a
    // MSG_CHANNEL_COOKIE instead of a MSG_CHANNEL_ACKNOWLEDGE.
    auto data = m_channels.get(full_id);
    if (data && data->has_egress_data_pending()) {
      // TODO MSG_CHANNEL_COOKIE
      //      https://gitlab.com/interpeer/channeler/-/issues/13
      LIBLOG_DEBUG("Sending MSG_CHANNEL_COOKIE: " << full_id);
    }
    else {
      // MSG_CHANNEL_ACKNOWLEDGE
      LIBLOG_DEBUG("Sending MSG_CHANNEL_ACKNOWLEDGE: " << full_id
          << " with cookie1 " << std::hex << msg->cookie1
          << " and cookie2 " << cookie2 << std::dec);
      auto response = std::make_unique<message_channel_acknowledge>(full_id, msg->cookie1, cookie2);
      auto ev = std::make_unique<channeler::pipe::message_out_event>(
            packet.channel(), // XXX should be DEFAULT_CHANNELID
            std::move(response)
          );
      output_events.push_back(std::move(ev));
    }

    return true;
  }


  inline bool handle_finalize(message_channel_finalize * msg,
      ::channeler::packet_wrapper const & packet,
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    LIBLOG_DEBUG("MSG_CHANNEL_FINALIZE(channel["
        << std::hex << msg->id << "]/"
        << "cookie2[" << std::hex << msg->cookie2 << "]/"
        << "capabilities[" << std::hex << msg->capabilities << "])"
        << std::dec);

    // If we got a MSG_CHANNEL_NEW, we have one of three possible situations to
    // consider:
    // a) We already have a pending channel with this partial identifier. That
    //    means we have ourselves sent out a MSG_CHANNEL_NEW with this
    //    initiator part. That means we have some wires crossed and should
    //    resend the MSG_CHANNEL_NEW with a new initiator. But that is outside
    //    of the scope of this class, so we should drop the pending channel
    //    information instead. The channel_initiator class should retry with
    //    a new partial.
    // b) We have an established channel. That can only happen if we already
    //    received a MSG_CHANNEL_FINALIZE or equivalent MSG_CHANNEL_COOKIE. So
    //    the peer wants to re-establish the same channel, effectively. We can
    //    respond, but we need to re-use the same channel identifier.
    //    XXX It's probably wise to have a time stamp in the channel data so we
    //        can avoid being flooded here from a malicious peer.
    // c) We know nothing about this channel identifier. That's great, then we
    //    check the cookie and create channel data if the cookie matches.
    if (m_channels.has_pending_channel(msg->id.initiator)) {
      m_channels.drop_pending_channel(msg->id.initiator);
      // TODO notify other channel_initiator via action?
      LIBLOG_ERROR("Received a finalize for a pending channel; we'll abort.");
      return false;
    }

    // If the full channel exists, we're done. We just ignore the message entirely.
    if (m_channels.has_established_channel(msg->id)) {
      LIBLOG_DEBUG("Ignoring finalize; the channel is already established.");
      return true;
    }

    // If neither pending nor full channel exists, we need to check the cookie.
    // All the information for checking the cookie is available to us; it should
    // be the exact same cookie we sent ourselves.
    // XXX: Note that the secret generator could have shifted state between us
    //      sending the cookie, and processing this message. In that case, the
    //      cookie check will fail.
    auto secret = m_secret_generator();

    auto cookie = create_cookie_responder(secret.data(), secret.size(),
        packet.sender(), packet.recipient(), msg->id);

    // Check the cookies match.
    if (msg->cookie2 != cookie) {
      // TODO report this?
      // https://gitlab.com/interpeer/channeler/-/issues/17
      LIBLOG_ERROR("Ignoring finalize due to mismatching cookie: " << msg->id
          << " calculated: " << std::hex << cookie << " but got "
          << msg->cookie2 << std::dec);
      return false;
    }

    // We have a cookie match, which means we can add this channel id to our
    // channel set.
    auto err = m_channels.add(msg->id);
    if (ERR_SUCCESS != err) {
      // TODO report this?
      // https://gitlab.com/interpeer/channeler/-/issues/17
      LIBLOG_ET("Could not add channel: " << msg->id, err);
      return false;
    }

    LIBLOG_DEBUG("Channel fully established: " << msg->id);
    result_actions.push_back(std::move(
        std::make_unique<::channeler::pipe::notify_channel_established_action>(msg->id)
    ));

    // Add timeout TODO
    // m_timeouts.add({CHANNEL_TIMEOUT_TAG, msg->id.initiator},
    //     ::channeler::support::timeouts::duration{CHANNEL_TIMEOUT});

    return true;
  }


  inline bool handle_cookie(message_channel_cookie * msg [[maybe_unused]],
      ::channeler::packet_wrapper const & packet [[maybe_unused]],
      ::channeler::pipe::action_list_type & result_actions [[maybe_unused]],
      ::channeler::pipe::event_list_type & output_events [[maybe_unused]])
  {
    LIBLOG_DEBUG("TODO: MSG_CHANNEL_COOKIE");
    // TODO
    // https://gitlab.com/interpeer/channeler/-/issues/13
    return true;
  }



  virtual ~fsm_channel_responder() = default;

private:
  channel_set &     m_channels;
  secret_generator  m_secret_generator;
};


} // namespace channeler::fsm

#endif // guard
