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
#ifndef CHANNELER_PIPE_CALLBACK_H
#define CHANNELER_PIPE_CALLBACK_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <functional>

#include "../event.h"
#include "../action.h"
#include "../event_as.h"

namespace channeler::pipe {

/**
 * The callback filter breaks the compile-time dependencies, and introduces a callback
 * for pipeline events.
 */
template <
  typename channelT
>
struct callback_filter
{
  using input_event = packet_out_enqueued_event<channelT>;
  using channel_set = ::channeler::channels<channelT>;
  using channel_ptr = typename channel_set::channel_ptr;

  using callback = std::function<action_list_type (std::unique_ptr<event>)>;

  inline callback_filter(callback cb)
    : m_callback{cb}
  {
  }


  inline action_list_type consume(std::unique_ptr<event> ev)
  {
    event_assert_set("egress:callback", ev.get());
    return m_callback(std::move(ev));
  }

  callback  m_callback;
};


} // namespace channeler::pipe

#endif // guard
