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
#ifndef CHANNELER_PIPE_VALIDATE_H
#define CHANNELER_PIPE_VALIDATE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <set>

#include "../memory/packet_pool.h"
#include "event.h"
#include "action.h"

#include <channeler/packet.h>
#include <channeler/error.h>


namespace channeler::pipe {

/**
 * Null policy - does nothing, but provides the right kind of interface.
 *
 * See validate_filter for details.
 */
template <
  typename addressT = peerid
>
struct null_policy
{
  // FIXME ingress as an enum? see action.h
  inline bool should_filter(addressT const &, bool ingress [[maybe_unused]])
  {
    return false;
  }
};



/**
 * Validate packets.
 *
 * On the face of it, this filter merely validates packet checksums or similar
 * cryptographic mechanisms in order to decide whether a packet should be
 * processed further.
 *
 * But beyond that, this is also the place in which it should be decided
 * what consequences receiving an invalid packet has.
 *
 * If a packet is known to be invalid, the filter consults the peer failure
 * policy to decide whether to institute filtering at the level of the peerid,
 * and the transport failure policy whether to filter at the transport level.
 *
 * Expects the next_eventT constructor to take
 * - transport source address
 * - transport destination address
 * - packet_wrapper
 * - a pool slot
 *
 * such as e.g. decrypted_packet_event
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT,
  typename peer_failure_policyT = null_policy<peerid_wrapper>,
  typename transport_failure_policyT = null_policy<addressT>
>
struct validate_filter
{
  using input_event = decrypted_packet_event<addressT, POOL_BLOCK_SIZE>;

  inline validate_filter(next_filterT * next,
      peer_failure_policyT * peer_p = nullptr,
      transport_failure_policyT * trans_p = nullptr)
    : m_next{next}
    , m_peer_policy{peer_p}
    , m_transport_policy{trans_p}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_DECRYPTED_PACKET) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event * in = reinterpret_cast<input_event *>(ev.get());

    // If there is no data passed, we should also throw.
    if (nullptr == in->data.data()) {
      throw exception{ERR_INVALID_REFERENCE};
    }

    // We need to validate the packet. For now, this just means verifying the
    // checksum.
    if (!in->packet.has_valid_checksum()) {
      action_list_type res;
      if (m_peer_policy) {
        if (m_peer_policy->should_filter(in->packet.sender(), true)) {
          res.push_back(std::make_shared<peer_filter_request_action>(
                  in->packet.sender(), true));
        }
        if (m_peer_policy->should_filter(in->packet.recipient(), false)) {
          res.push_back(std::make_shared<peer_filter_request_action>(
                  in->packet.recipient(), false));
        }
      }

      if (m_transport_policy) {
        if (m_transport_policy->should_filter(in->transport.source, true)) {
          res.push_back(std::make_shared<transport_filter_request_action<addressT>>(
                in->transport.source, true));
        }
        if (m_transport_policy->should_filter(in->transport.destination, false)) {
          res.push_back(std::make_shared<transport_filter_request_action<addressT>>(
                in->transport.destination, false));
        }
      }

      // If the checksum is invalid, we know we want to exit the pipe here.
      return res;
    }

    // At the next filter, we require full packets again. Let's just move
    // the event, then.
    return m_next->consume(std::move(ev));
  }


  next_filterT *              m_next;
  peer_failure_policyT *      m_peer_policy;
  transport_failure_policyT * m_transport_policy;
};


} // namespace channeler::pipe

#endif // guard
