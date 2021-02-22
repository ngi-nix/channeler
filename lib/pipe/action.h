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
#ifndef CHANNELER_PIPE_ACTION_H
#define CHANNELER_PIPE_ACTION_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <list>
#include <memory>

#include <channeler/peerid.h>

namespace channeler::pipe {


/**
 * Actions percolate up the filter pipe. They have a type, and may carry
 * a type-dependent payload.
 */
enum action_type : uint_fast16_t
{
  AT_UNKNOWN = 0,
  AT_FILTER_TRANSPORT,
  AT_FILTER_PEER,
};



/**
 * Action base class
 */
struct action
{
  action_type const type = AT_UNKNOWN;
};

// Action list type
using action_list_type = std::list<std::unique_ptr<action>>;


/**
 * Action for requesting filtering on different address types.
 */
template <typename addressT>
struct transport_filter_request_action
  : public action
{
  addressT address;
  bool     ingress = true; // FIXME enum?

  inline transport_filter_request_action(addressT const & addr)
    : action{AT_FILTER_TRANSPORT}
    , address{addr}
  {
  }
};

struct peer_filter_request_action
  : public action
{
  peerid peer;
  bool   ingress = true; // FIXME enum?

  inline peer_filter_request_action(peerid_wrapper const & p)
    : action{AT_FILTER_PEER}
    , peer{p.copy()}
  {
  }
};


} // namespace channeler::pipe

#endif // guard