/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2020 Jens Finkhaeuser.
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
#ifndef CHANNELER_INTERNAL_API_H
#define CHANNELER_INTERNAL_API_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <functional>

#include <channeler/channelid.h>
#include <channeler/error.h>

#include "../macros.h"

#include "../fsm/default.h"
#include "../pipe/ingress.h"
#include "../pipe/egress.h"
#include "../pipe/action.h"

namespace channeler::internal {

/**
 * This file contains the *internal* API for channeler, i.e. an API that is
 * allows the user to add channeler's protocol support to an existing
 * connection management system.
 *
 * The API requires a context which encapsulates all the platform support
 * the stack requires.
 *
 * The expected usage of this class is for the caller to establish a
 * connection of sorts, and to instanciate one of these classes per
 * connection.
 */
template <
  typename connection_contextT
>
struct connection_api
{
  using pool_type = typename connection_contextT::node_type::pool_type;
  using slot_type = typename pool_type::slot;

  using address_type = typename connection_contextT::address_type;

  using ingress_type = pipe::default_ingress<connection_contextT>;
  using egress_type = pipe::default_egress<connection_contextT>;

  using channel_establishment_callback = std::function<void (error_t, channelid const &)>;
  using packet_to_send_callback = std::function<void (channelid const &)>;
  using data_available_callback = std::function<void (channelid const &, std::size_t)>;

  using buffer_entry = typename connection_contextT::channel_type::buffer_type::buffer_entry;

  /**
   * Constructor accepts:
   * TODO
   */
  inline connection_api(connection_contextT & context,
      channel_establishment_callback remote_cb,
      packet_to_send_callback packet_cb,
      data_available_callback data_cb
    )
    : m_context{context}
    , m_registry{fsm::get_standard_registry<typename connection_contextT::address_type>(m_context)}
    , m_event_route_map{}
    , m_ingress{m_registry, m_event_route_map, m_context.channels()}
    , m_egress{
        std::bind(&connection_api::redirect_egress_event, this, std::placeholders::_1),
        m_context.channels(),
        m_context.node().packet_pool(),
        [this]() { return m_context.node().id(); },
        [this]() { return m_context.peer(); }
      }
    , m_remote_establishment_cb{remote_cb}
    , m_packet_to_send_cb{packet_cb}
    , m_data_available_cb{data_cb}
  {
    // Populate event route map
    using namespace std::placeholders;

    m_event_route_map[pipe::EC_UNKNOWN] =
      std::bind(&connection_api::handle_unknown_event, this, _1);

    m_event_route_map[pipe::EC_INGRESS] =
      std::bind(&connection_api::handle_ingress_event, this, _1);

    m_event_route_map[pipe::EC_EGRESS] =
      std::bind(&connection_api::handle_egress_event, this, _1);

    m_event_route_map[pipe::EC_USER] =
      std::bind(&connection_api::handle_user_event, this, _1);

    m_event_route_map[pipe::EC_SYSTEM] =
      std::bind(&connection_api::handle_system_event, this, _1);

    m_event_route_map[pipe::EC_NOTIFICATION] =
      std::bind(&connection_api::handle_notification_event, this, _1);
  }

  // *** Channel interface

  /**
   * Initiate a channel.
   *
   * ERR_SUCCESS means the initial establishment message was sent properly,
   * and does not indicate overall success. On overall success,
   * the channel establishment callback will be invoked.
   *
   * Note that this does not permit the user to tie a *purpose* to a particular
   * channel, which is less than ideal.
   *
   * TODO: also store a user-provided tag in channel data, so that the
   *       establishment callback can be used to tie a new channel back to
   *       the user's action.
   */
  inline error_t establish_channel(peerid const & peer)
  {
    // Channel establishment happens on the default channel; we need to create a
    // default channel entry if we haven't already.
    m_context.channels().add(DEFAULT_CHANNELID);

    // Establishing a channel means sending an appropriate user
    // event to the FSM.
    auto event = pipe::new_channel_event(m_context.node().id(), peer);

    pipe::action_list_type result_actions;
    pipe::event_list_type result_events;
    auto processed = m_registry.process(&event, result_actions, result_events);
    if (!processed || result_events.empty()) {
      return ERR_STATE;
    }

    auto ev = std::move(result_events.front());
    result_events.pop_front(); // XXX necessary? probably not.
    if (!ev) {
      LIBLOG_ERROR("Registry did not produce a result event!");
      return ERR_STATE;
    }

    // This *should* produce a message out event. If that's not happening, it's unclear
    // what we should do.
    if (ev->type != pipe::ET_MESSAGE_OUT) {
      LIBLOG_ERROR("Registry dit not produce an outgoing message!");
      return ERR_STATE;
    }

    // Since this is ET_MESSAGE_OUT, feed this to the egress pipe. The pipe
    // then produces callbacks as necessary.
    result_actions = m_egress.consume(std::move(ev));
    if (!result_actions.empty()) {
      // TODO handle better
      LIBLOG_ERROR("TODO");
      return ERR_UNEXPECTED;
    }

    LIBLOG_DEBUG("Channel established successfully.");
    return ERR_SUCCESS;
  }


  /**
   * Write data to a channel.
   *
   * This is raw application data; it will be wrapped in a message, packet,
   * etc. by this API.
   *
   * Note that for simplicity's sake, this API does *not* currently break down
   * too-large data chunks into individual packets. TODO
   */
  inline error_t channel_write(channelid const & id, std::byte const * data,
      std::size_t length, std::size_t & written)
  {
    if (id == DEFAULT_CHANNELID || !id.has_responder()) {
      return ERR_INVALID_CHANNELID;
    }

    // TODO
    // - feed to FSM as user data
    // - feed the resulting data message to egress pipe
    return ERR_UNEXPECTED;

    return ERR_SUCCESS;
  }

  inline error_t channel_write(channelid const & id, char const * data,
      std::size_t length, std::size_t & written)
  {
    return channel_write(id,
        reinterpret_cast<std::byte const *>(data),
        length, written);
  }



  /**
   * Read data from channel.
   *
   * Again, this is raw application data.
   */
  inline error_t channel_read(channelid const & id, std::byte * data,
      std::size_t max, std::size_t & read)
  {
    // TODO
    // - If channel data has data available, return it.
    return ERR_UNEXPECTED;

    return ERR_SUCCESS;
  }

  inline error_t channel_read(channelid const & id, char * data,
      std::size_t max, std::size_t & read)
  {
    return channel_read(id,
        reinterpret_cast<std::byte *>(data),
        max, read);
  }


  // *** I/O interface

  /**
   * Allocate a packet slot.
   *
   * TODO ingress, egress or both?
   */
  inline slot_type allocate()
  {
    return m_context.node().packet_pool().allocate();
  }


  /**
   * Consume received packets.
   *
   * Buffers for incoming packets are taken from a pool, so the API
   * really just needs a pool slot.
   */
  inline error_t received_packet(address_type const & source,
      address_type const & destination,
      slot_type const & slot)
  {
    LIBLOG_DEBUG("Received packet: " << slot.size());
    auto ev = std::make_unique<
      pipe::raw_buffer_event<
        address_type, connection_contextT::POOL_BLOCK_SIZE
      >
    >(source, destination, slot);

    // Feed into default ingress pipe
    auto actions = m_ingress.consume(std::move(ev));

    for (auto & act : actions) {
      // We cannot handle all actions. However, we do expect a channel
      // establishment notification action here.
      // TODO also: error
      switch (act->type) {
        case pipe::AT_NOTIFY_CHANNEL_ESTABLISHED:
          {
            auto actconv = reinterpret_cast<pipe::notify_channel_established_action *>(act.get());
            LIBLOG_DEBUG("FSM reports channel established: " << actconv->channel);
            m_remote_establishment_cb(ERR_SUCCESS, actconv->channel);
          }
          break;

        default:
          LIBLOG_ERROR("Ingress pipe reports action we don't understand: "
              << act->type);
          return ERR_UNEXPECTED;
      }
    }

    LIBLOG_DEBUG("Packet processed after receipt.");
    return ERR_SUCCESS;
  }


  /**
   * Dequeue packet ready for sending.
   *
   * Also buffers for outgoing packets are taken from a pool, so the
   * API just returns the slot. If the slot is empty, there is no more
   * data ready for sending.
   */
  inline buffer_entry packet_to_send(channelid const & channel)
  {
    auto ptr = m_context.channels().get(channel);
    // TODO in future, don't just pop - we might have to resend something
    return ptr->egress_buffer_pop();
  }


private:

  pipe::action_list_type redirect_egress_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Egress event produced: " << ev->category << " / " << ev->type);
    switch (ev->type) {
      case pipe::ET_PACKET_OUT_ENQUEUED:
        {
          auto converted = reinterpret_cast<
            pipe::packet_out_enqueued_event<typename connection_contextT::channel_type> *
          >(ev.get());
          auto channel = converted->channel->id();
          LIBLOG_DEBUG("Notifying packet available on channel: " << channel);
          m_packet_to_send_cb(channel);
        }
        break;

      default:
        break;
    }

    // TODO
    return {};
  }

  pipe::action_list_type handle_ingress_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling ingress event of type: " << ev->type);
    return {};
  }

  pipe::action_list_type handle_unknown_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling unknown event of type: " << ev->type);
    return {};
  }

  pipe::action_list_type handle_egress_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling egress event of type: " << ev->type);
    return m_egress.consume(std::move(ev));
  }

  pipe::action_list_type handle_user_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling user event of type: " << ev->type);
    return {};
  }

  pipe::action_list_type handle_system_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling system event of type: " << ev->type);
    return {};
  }

  pipe::action_list_type handle_notification_event(std::unique_ptr<pipe::event> ev)
  {
    LIBLOG_DEBUG("Handling notification event of type: " << ev->type);
    return {};
  }



  connection_contextT &           m_context;
  fsm::registry                   m_registry;

  pipe::event_route_map           m_event_route_map;

  ingress_type                    m_ingress;
  egress_type                     m_egress;

  channel_establishment_callback  m_remote_establishment_cb;
  packet_to_send_callback         m_packet_to_send_cb;
  data_available_callback         m_data_available_cb;
};


} // namespace channeler::internal

#endif // guard
