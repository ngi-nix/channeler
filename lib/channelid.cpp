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
#include <build-config.h>

#include <channeler/channelid.h>

#include "support/random_bits.h"

namespace channeler {

channelid create_new_channelid()
{
  support::random_bits<uint16_t> rng;

  uint16_t cur;
  do {
    cur = rng.get();
  } while (cur == DEFAULT_CHANNELID.initiator);

  channelid id{};
  id.initiator = cur;
  return id;
}



error_t complete_channelid(channelid & id)
{
  // We can only complete a channel identifier if
  // a) the initiator part *is not* default, and
  // b) the responder part *is* default.
  if (!id.has_initiator()) {
    return ERR_INVALID_CHANNELID;
  }
  if (id.has_responder()) {
    return ERR_INVALID_CHANNELID;
  }

  // Generate
  support::random_bits<uint16_t> rng;

  uint16_t cur;
  do {
    cur = rng.get();
  } while (cur == DEFAULT_CHANNELID.initiator);

  id.responder = cur;
  return ERR_SUCCESS;
}


} // namespace channeler
