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

#include <random>
#include <chrono>
#include <limits>

// FIXME
//#include <channeler/error.h>
//
//#include <cstring>
//
//#include <stdexcept>
//#include <random>
//
//#include <liberate/string/hexencode.h>
//#include <liberate/cpp/hash.h>

namespace channeler {

namespace {

template <typename T>
struct random_bits
{
  std::default_random_engine generator;
  std::uniform_int_distribution<T> distribution;

  inline random_bits()
    : generator{
        static_cast<std::default_random_engine::result_type>(
            std::chrono::system_clock::now().time_since_epoch().count()
        )
      }
    , distribution{
        std::numeric_limits<T>::min(),
        std::numeric_limits<T>::max()
      }
  {
  }

  T get()
  {
    return distribution(generator);
  }
};


} // anonymous namespace



channelid create_new_channelid()
{
  random_bits<uint16_t> rng;

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
  random_bits<uint16_t> rng;

  uint16_t cur;
  do {
    cur = rng.get();
  } while (cur == DEFAULT_CHANNELID.initiator);

  id.responder = cur;
  return ERR_SUCCESS;
}


} // namespace channeler
