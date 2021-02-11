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
#ifndef CHANNELER_PEERID_H
#define CHANNELER_PEERID_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <cstddef>

#include <liberate/cpp/operators/comparison.h>
#include <liberate/cpp/hash.h>

namespace channeler {

// Size of a peer identifier
constexpr size_t const PEERID_SIZE_BYTES = 16;
constexpr size_t const PEERID_SIZE_BITS = PEERID_SIZE_BYTES * 8;

/**
 * For now, a peer identifier is a fixed length byte string that is largely
 * opaque to the protocol.
 */
struct CHANNELER_API peerid
  : public ::liberate::cpp::comparison_operators<peerid>
{
  std::byte buffer[PEERID_SIZE_BYTES];

  // Create random peer identifier
  peerid();

  // Read a peer identifier from a buffer
  peerid(std::byte const * buf, size_t bufsize);
  peerid(char const * buf, size_t bufsize);

  std::string display() const;
  size_t hash() const;

  bool is_equal_to(peerid const & other) const;
  bool is_less_than(peerid const & other) const;
};

} // namespace channeler

LIBERATE_MAKE_HASHABLE(channeler::peerid)


#endif // guard
