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
#include <channeler/error.h>

#include <cstring>

#include <stdexcept>
#include <random>
#include <limits>
#include <chrono>

#include <liberate/string/hexencode.h>
#include <liberate/cpp/hash.h>

namespace channeler {


peerid_wrapper::peerid_wrapper(std::byte * start, size_t bufsize)
  : raw{start}
{
  if (bufsize < size()) {
    throw exception{ERR_INSUFFICIENT_BUFFER_SIZE,
      "Input buffer too small for a peer identifier."};
  }
}


std::string
peerid_wrapper::display() const
{
  std::string res = "0x";
  return res + liberate::string::hexencode(raw, PEERID_SIZE_BYTES);
}



size_t
peerid_wrapper::hash() const
{
  return liberate::cpp::range_hash(raw, raw+ PEERID_SIZE_BYTES);
}



bool
peerid_wrapper::is_equal_to(peerid_wrapper const & other) const
{
  return (0 == ::memcmp(raw, other.raw, PEERID_SIZE_BYTES));
}



bool
peerid_wrapper::is_less_than(peerid_wrapper const & other) const
{
  return (0 > ::memcmp(raw, other.raw, PEERID_SIZE_BYTES));
}



peerid
peerid_wrapper::copy() const
{
  return {raw, size()};
}



peerid::peerid()
  : peerid_wrapper{buffer, PEERID_SIZE_BYTES}
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
  : peerid_wrapper{buffer, bufsize}
{
  ::memcpy(buffer, buf, PEERID_SIZE_BYTES);
}



peerid::peerid(char const * buf, size_t bufsize)
  : peerid_wrapper{buffer, sizeof(buffer)}
{
  char const * start = buf;
  size_t buflen = bufsize;
  if (bufsize > 2 && buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
    start += 2;
    buflen -= 2;
    if (bufsize < (PEERID_SIZE_BYTES * 2) + 2) {
      throw exception{ERR_INSUFFICIENT_BUFFER_SIZE,
        "Peer identifier buffer is too small."};
    }
  }
  else {
    if (bufsize < PEERID_SIZE_BYTES * 2) {
      throw exception{ERR_INSUFFICIENT_BUFFER_SIZE,
        "Peer identifier buffer is too small."};
    }
  }

  auto used = liberate::string::hexdecode(buffer, PEERID_SIZE_BYTES,
      reinterpret_cast<std::byte const *>(start), buflen);
  if (used != PEERID_SIZE_BYTES) {
    throw exception{ERR_DECODE,
      "Could not decode hexadecimal peer identifier."};
  }
}


} // namespace channeler
