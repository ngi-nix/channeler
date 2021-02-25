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
#ifndef CHANNELER_CAPABILITIES_H
#define CHANNELER_CAPABILITIES_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <bitset>

namespace channeler {

/**
 * Capabilities representation
 */
using capability_bits_t = uint16_t;
constexpr size_t CAPABILITY_COUNT = sizeof(capability_bits_t) * 8;
using capabilities_t = std::bitset<CAPABILITY_COUNT>;

/**
 * Capabilities
 *
 * This enum provides names for indices into the capability bitset.
 *
 * Note that capability indices in bitsets are LSB to MSB.
 */
enum capability_index : std::size_t
{
  // Resend lost packets.
  CAP_RESEND = 0,

  // Strict packet ordering
  CAP_ORDERED = 1,

  // Close-on-loss behaviour. Note that "loss" is defined as the final state
  // when all resend attempts have failed.
  CAP_CLOSE_ON_LOSS = 2,
};


} // namespace channeler


#endif // guard
