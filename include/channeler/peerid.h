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
#include <ostream>
#include <iomanip>

#include <liberate/cpp/operators/comparison.h>
#include <liberate/cpp/hash.h>

namespace channeler {

// Size of a peer identifier
constexpr size_t const PEERID_SIZE_BYTES = 16;
constexpr size_t const PEERID_SIZE_BITS = PEERID_SIZE_BYTES * 8;

/**
 * Forward declaration.
 */
struct CHANNELER_API peerid;

/**
 * This class wraps a memory region and interprets it as a peer identifier.
 * Note that the memory region must be PEERID_SIZE_BYTES in size; the wrapper
 * class does not perform any range checks.
 *
 * For now, a peer identifier is a fixed length byte string that is largely
 * opaque to the protocol.
 */
struct CHANNELER_API peerid_wrapper
  : public ::liberate::cpp::comparison_operators<peerid_wrapper>
{
  /**
   * Create a peerid_wrapper using the peer identifier in the given byte
   * buffer. The required buffer size is a constexpr, and the passed buffer
   * size is checked for sufficient size.
   */
  peerid_wrapper(std::byte const * start, size_t bufsize);

  std::string display() const;
  size_t hash() const;

  bool is_equal_to(peerid_wrapper const & other) const;
  bool is_less_than(peerid_wrapper const & other) const;

  static constexpr size_t size()
  {
    return PEERID_SIZE_BYTES;
  }

  // Create a copy of the wrapped data
  peerid copy() const;

  // Pointer to raw buffer; equivalent to start in the constructor
  std::byte const * raw = nullptr;

  peerid_wrapper(peerid_wrapper const & other);
  peerid_wrapper & operator=(peerid_wrapper const & other);
};


/**
 * Output operator
 */
inline std::ostream &
operator<<(std::ostream & os, peerid_wrapper const & id)
{
  os
    << std::hex
    << "<pid/";
  for (size_t idx = 0 ; idx < id.size() ; ++idx) {
    os << std::setw(2) << std::setfill('0')
      << static_cast<uint16_t>(id.raw[idx]);
  }
  os << ">" << std::dec;
  return os;
}




/**
 * By contrast, this class manages a buffer for a peer identifier. It shares
 * most of its implementation with peerid_wrapper.
 */
struct CHANNELER_API peerid
  : public peerid_wrapper
{
  std::byte buffer[PEERID_SIZE_BYTES];

  // New random peer identifier
  peerid();

  // Copy peer identifier from a buffer
  peerid(std::byte const * buf, size_t bufsize);
  peerid(char const * buf, size_t bufsize);

  peerid(peerid const &);
  peerid & operator=(peerid const &);
};



} // namespace channeler

LIBERATE_MAKE_HASHABLE(channeler::peerid_wrapper)


#endif // guard
