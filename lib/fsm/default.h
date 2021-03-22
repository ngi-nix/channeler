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
#ifndef CHANNELER_FSM_DEFAULT_H
#define CHANNELER_FSM_DEFAULT_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include "base.h"
#include "channel_initiator.h"
#include "channel_responder.h"
#include "data.h"

namespace channeler::fsm {

/**
 * Return a pre-configured registry. We may have different versions of these, or
 * pass some kind of enum as input in future.
 *
 * We'll pass this a context; the context contains all type definitions, etc.
 * necessary for constructing FSMs.
 *
 * TODO
 */
template <
  typename contextT
>
inline registry<contextT>
get_standard_registry(contextT & ctx)
{
  registry<contextT> reg{&ctx};

  // Channel initiator
  using init_fsm_t = fsm_channel_initiator<
    typename contextT::address_type,
    contextT::POOL_BLOCK_SIZE,
    typename contextT::channel_type
  >;
  auto init = std::make_unique<init_fsm_t>(
      ctx.timeouts, ctx.channels, ctx.secret_generator
  );
  reg.add_move(std::move(init));

  // Channel responder
  using resp_fsm_t = fsm_channel_responder<
    typename contextT::address_type,
    contextT::POOL_BLOCK_SIZE,
    typename contextT::channel_type
  >;
  auto resp = std::make_unique<resp_fsm_t>(
      ctx.channels, ctx.secret_generator
  );
  reg.add_move(std::move(resp));

  // Data
  using data_fsm_t = fsm_data<
    typename contextT::address_type,
    contextT::POOL_BLOCK_SIZE,
    typename contextT::channel_type
  >;
  auto data = std::make_unique<data_fsm_t>(ctx.channels);
  reg.add_move(std::move(data));

  return reg;
}

} // namespace channeler::fsm

#endif // guard
