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

#include <channeler/cookie.h>

#include <cstring>
#include <vector>

#include <liberate/serialization/integer.h>
#include <liberate/checksum/crc32.h>

namespace channeler {


cookie
create_cookie_initiator(
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid::half_type initiator_part)
{
  // Buffer for the cookie inputs
  std::vector<std::byte> buf{
    secret_size + (PEERID_SIZE_BYTES * 2) + sizeof(channelid::half_type)
  };

  // First the secret
  std::byte * offs = &buf[0];
  ::memcpy(offs, secret, secret_size);
  offs += secret_size;

  // Initiator and responder peer id
  ::memcpy(offs, initiator.raw, initiator.size());
  offs += initiator.size();

  ::memcpy(offs, responder.raw, responder.size());
  offs += responder.size();

  // Channel id part
  liberate::serialization::serialize_int(offs, buf.size() - (offs - buf.data()),
      initiator_part);

  // Checksum
  cookie res = liberate::checksum::crc32<liberate::checksum::CRC32>(
      buf.begin(), buf.end());
  return res;
}



cookie
create_cookie_responder(
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid const & id)
{
  // Buffer for the cookie inputs
  std::vector<std::byte> buf{
    secret_size + (PEERID_SIZE_BYTES * 2) + sizeof(channelid)
  };

  // First the secret
  std::byte * offs = &buf[0];
  if (secret_size > 0) {
    ::memcpy(offs, secret, secret_size);
    offs += secret_size;
  }

  // Initiator and responder peer id
  ::memcpy(offs, initiator.raw, initiator.size());
  offs += initiator.size();

  ::memcpy(offs, responder.raw, responder.size());
  offs += responder.size();

  // Channel id part
  liberate::serialization::serialize_int(offs, buf.size() - (offs - buf.data()),
      id.full);

  // Checksum
  cookie res = liberate::checksum::crc32<liberate::checksum::CRC32>(
      buf.begin(), buf.end());
  return res;
}



} // namespace channeler
