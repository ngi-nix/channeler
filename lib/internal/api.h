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

#include "../fsm/default.h"

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
  typename contextT
>
struct connection_api
{
  using pool_type = typename contextT::pool_type;
  using slot_type = typename pool_type::slot;

  using channel_establishment_callback = std::function<void (error_t, channelid const &)>;
  using packet_to_send_callback = std::function<void ()>;
  using data_available_callback = std::function<void (channelid const &, std::size_t)>;

  /**
   * Constructor accepts:
   *
   * - a context, providing per-connection  FIXME
   * TODO
   */
  inline connection_api(contextT & context,
      channel_establishment_callback remote_cb,
      packet_to_send_callback packet_cb,
      data_available_callback data_cb
    )
    : m_context{context}
    , m_registry{fsm::get_standard_registry(m_context)}
    , m_remote_establishment_cb{remote_cb}
    , m_packet_to_send_cb{packet_cb}
    , m_data_available_cb{data_cb}
  {
  }

  // *** Channel interface

  /**
   * Initiate a channel.
   *
   * The callback will be invoked with an error code; if ERR_SUCCESS is
   * reported, the channelid parameter contains the id of the newly
   * established channel.
   */
  inline error_t establish_channel(peerid const & peer, channel_establishment_callback cb)
  {
    // Establishing a channel means sending an appropriate user
    // event to the FSM.
    auto event = pipe::new_channel_event(m_context.id, peer);

    pipe::action_list_type result_actions;
    pipe::event_list_type result_events;
    auto processed = m_registry.process(&event, result_actions, result_events);
    if (!processed) {
      return ERR_STATE;
    }

    // TODO
    // - send produced message to output pipe to produce packet (eventually)
    return ERR_UNEXPECTED;

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
    return m_context.packet_pool.allocate();
  }


  /**
   * Consume received packets.
   *
   * Buffers for incoming packets are taken from a pool, so the API
   * really just needs a pool slot.
   */
  inline error_t received_packet(slot_type slot)
  {
    // TODO
    // Feed into default ingress pipe
    return ERR_UNEXPECTED;
  }


  /**
   * Dequeue packet ready for sending.
   *
   * Also buffers for outgoing packets are taken from a pool, so the
   * API just returns the slot. If the slot is empty, there is no more
   * data ready for sending.
   */
  inline slot_type packet_to_send()
  {
    // TODO
  }


private:
  using registry_type = fsm::registry<contextT>;

  contextT &    m_context;
  registry_type m_registry;

  channel_establishment_callback  m_remote_establishment_cb;
  packet_to_send_callback         m_packet_to_send_cb;
  data_available_callback         m_data_available_cb;
};


} // namespace channeler::internal

#endif // guard
