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
#ifndef CHANNELER_PIPE_ROUTE_H
#define CHANNELER_PIPE_ROUTE_H

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
 * Route packets.
 *
 * For the time being, this means dropping packets with unacceptable source
 * or destination addresses.
 *
 * Expects the next_eventT constructor to take
 * - transport source address
 * - transport destination address
 * - packet_wrapper
 * - a pool slot
 *
 * such as e.g. decrypted_packet_event
 *
 * TODO:
 * - unban action or interface
 * - timeout of bans
 *   https://gitlab.com/interpeer/channeler/-/issues/24
 */
template <
  typename addressT,
  std::size_t POOL_BLOCK_SIZE,
  typename next_filterT,
  typename next_eventT
>
struct route_filter
{
  using input_event = parsed_header_event<addressT, POOL_BLOCK_SIZE>;

  inline route_filter(next_filterT * next)
    : m_next{next}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    if (!ev) {
      throw exception{ERR_INVALID_REFERENCE};
    }
    if (ev->type != ET_PARSED_HEADER) {
      throw exception{ERR_INVALID_PIPE_EVENT};
    }

    input_event * in = reinterpret_cast<input_event *>(ev.get());

    // If there is no data passed, we should also throw.
    if (nullptr == in->data.data()) {
      throw exception{ERR_INVALID_REFERENCE};
    }

    // Do the actual filtering.
    peerid id = in->header.sender.copy();
    if (m_sender_banlist.find(id) != m_sender_banlist.end()) {
      return {};
    }
    id = in->header.recipient.copy();
    if (m_recipient_banlist.find(id) != m_recipient_banlist.end()) {
      return {};
    }

    // At the next event, we expect a packet not just a header. This means
    // parsing the header a second time for now.
    // TODO: new packet constructor
    // https://gitlab.com/interpeer/channeler/-/issues/25
    auto next = std::make_unique<next_eventT>(
        in->transport.source,
        in->transport.destination,
        channeler::packet_wrapper{in->data.data(), in->data.size()},
        in->data);
    auto res = m_next->consume(std::move(next));

    // We do have to handle some action types.
    for (auto & action : res) {
      if (AT_FILTER_PEER == action->type) {
        auto act = reinterpret_cast<peer_filter_request_action *>(action.get());
        if (act->ingress) {
          m_sender_banlist.insert(act->peer);
        }
        else {
          m_recipient_banlist.insert(act->peer);
        }
      }
    }

    return res;
  }


  next_filterT *  m_next;

  // The route filter currently mostly drops packets if their source or
  // destination peer address is not acceptable.
  std::set<peerid>  m_sender_banlist;
  std::set<peerid>  m_recipient_banlist;
};


} // namespace channeler::pipe

#endif // guard
