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

#include <channeler/peerid.h>

#include <cstring>

#include <stdexcept>
#include <random>
#include <limits>
#include <chrono>

#include <liberate/string/hexencode.h>
#include <liberate/cpp/hash.h>

namespace channeler {

peerid::peerid()
{
  // XXX When we have a crypto library, this should no longer be necessary.
  //     For one thing, we probably don't want random peerids any longer, but
  //     for another, we can also use a secure random generator.
  auto seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<unsigned char> distribution(0,
      std::numeric_limits<unsigned char>::max());

  std::byte * offset = buffer;
  while (offset < buffer + PEERID_SIZE_BYTES) {
    *offset = static_cast<std::byte>(distribution(generator));
    ++offset;
  }
}



peerid::peerid(std::byte const * buf, size_t bufsize)
{
  if (bufsize < PEERID_SIZE_BYTES) {
    throw std::out_of_range("Peer identifier buffer is too small.");
  }

  ::memcpy(buffer, buf, PEERID_SIZE_BYTES);
}



peerid::peerid(char const * buf, size_t bufsize)
{
  char const * start = buf;
  size_t buflen = bufsize;
  if (bufsize > 2 && buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
    start += 2;
    buflen -= 2;
    if (bufsize < (PEERID_SIZE_BYTES * 2) + 2) {
      throw std::out_of_range("Peer identifier buffer is too small.");
    }
  }
  else {
    if (bufsize < PEERID_SIZE_BYTES * 2) {
      throw std::out_of_range("Peer identifier buffer is too small.");
    }
  }

  auto used = liberate::string::hexdecode(buffer, PEERID_SIZE_BYTES,
      reinterpret_cast<std::byte const *>(start), buflen);
  if (used != PEERID_SIZE_BYTES) {
    throw std::invalid_argument("Could not decode hexadecimal peer identifier.");
  }
}



std::string
peerid::display() const
{
  std::string res = "0x";
  return res + liberate::string::hexencode(buffer, PEERID_SIZE_BYTES);
}



size_t
peerid::hash() const
{
  return liberate::cpp::range_hash(&buffer[0], buffer + PEERID_SIZE_BYTES);
}



bool
peerid::is_equal_to(peerid const & other) const
{
  return (0 == ::memcmp(buffer, other.buffer, PEERID_SIZE_BYTES));
}



bool
peerid::is_less_than(peerid const & other) const
{
  return (0 > ::memcmp(buffer, other.buffer, PEERID_SIZE_BYTES));
}


} // namespace channeler
