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
#ifndef CHANNELER_SUPPORT_EXPONENTIAL_BACKOFF_H
#define CHANNELER_SUPPORT_EXPONENTIAL_BACKOFF_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <cmath>

#include "random_bits.h"

namespace channeler::support {

/**
 * Implement exponential backoff.
 *
 * Given a number of collisions (failures), return some multiplier for a
 * backoff factor, defined as an integer between 0 and (2^c - 1).
 */
inline std::size_t
backoff_multiplier(std::size_t const & collisions)
{
  random_bits<std::size_t> rng;
  std::size_t clamp = exp2(collisions) - 1;
  auto rand = rng.get_factor();
  auto ret = std::nearbyint(rand * clamp);
  return ret;
}


/**
 * The backoff function is statically parametrized with the backoff factor.
 * This is to provide for a simpler function prototype that just requires
 * the number of collisions, and returns the actual backoff value.
 */
template <
  typename backoffT,
  backoffT BACKOFF
>
inline backoffT
backoff(std::size_t const & collisions)
{
  return BACKOFF * backoff_multiplier(collisions);
}

} // namespace channeler::support


#endif // guard
