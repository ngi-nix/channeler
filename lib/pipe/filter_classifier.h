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
#ifndef CHANNELER_PIPE_FILTER_CLASSIFIER_H
#define CHANNELER_PIPE_FILTER_CLASSIFIER_H

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
  // https://gitlab.com/interpeer/channeler/-/issues/18
  inline bool should_filter(addressT const &, bool ingress [[maybe_unused]])
  {
    return false;
  }
};


/**
 * The filter classifier doesn't classify filters - that's a bit of a misnomer,
 * and maybe something to fix in future. TODO
 * https://gitlab.com/interpeer/channeler/-/issues/23
 *
 * Rather, it provides the classification algorithm for what to do with packets
 * that need to be rejected. More precisely, if a packet should be rejected, it
 * should be fed to this class - which produces an action_list based on the
 * specific policies it's parametrized with.
 *
 * The main purpose of this class is re-usability across different filters.
 */
template <
  typename addressT,
  typename peer_failure_policyT,
  typename transport_failure_policyT
>
struct filter_classifier
{
  inline filter_classifier(
      peer_failure_policyT * peer_p = nullptr,
      transport_failure_policyT * trans_p = nullptr)
    : m_peer_policy{peer_p}
    , m_transport_policy{trans_p}
  {
  }


  inline action_list_type process(
      addressT const & transport_source,
      addressT const & transport_destination,
      ::channeler::packet_wrapper const & packet)
  {
    action_list_type res;

    if (m_peer_policy) {
      if (m_peer_policy->should_filter(packet.sender(), true)) {
        res.push_back(std::make_shared<peer_filter_request_action>(
                packet.sender(), true));
      }
      if (m_peer_policy->should_filter(packet.recipient(), false)) {
        res.push_back(std::make_shared<peer_filter_request_action>(
                packet.recipient(), false));
      }
    }

    if (m_transport_policy) {
      if (m_transport_policy->should_filter(transport_source, true)) {
        res.push_back(std::make_shared<transport_filter_request_action<addressT>>(
              transport_source, true));
      }
      if (m_transport_policy->should_filter(transport_destination, false)) {
        res.push_back(std::make_shared<transport_filter_request_action<addressT>>(
              transport_destination, false));
      }
    }

    return res;
  }


  peer_failure_policyT *      m_peer_policy;
  transport_failure_policyT * m_transport_policy;
};


} // namespace channeler::pipe

#endif // guard
