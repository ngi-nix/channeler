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
#ifndef CHANNELER_COOKIE_H
#define CHANNELER_COOKIE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <channeler/channelid.h>
#include <channeler/peerid.h>

#include <liberate/checksum/crc32.h>

namespace channeler {

/**
 * Cookies are esnt in the channel establishment messages, which can also be
 * considered as the connection handshake.
 *
 * The purpose of cookies is to provide some kind of validation that some
 * secret is known - for that reason, something like an HMAC should be used.
 *
 * XXX For the time being, we're using a CRC32 checksum, which is not very
 *     secure. But it helps get the protocol off the ground without requiring
 *     a crypto library just yet.
 */
using cookie = liberate::checksum::crc32_checksum;

/**
 * We're using two cookies: one in the first part of the handshake, and one
 * in a latter. In the first part, the full channelid is not yet known. The
 * second part has the channel identifier.
 */
CHANNELER_API
cookie
create_cookie_initiator(
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid::half_type initiator_part);


CHANNELER_API
cookie
create_cookie_responder(
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid const & id);


/**
 * A very simple verification function.
 */
inline bool
validate_cookie(cookie const & c,
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid::half_type initiator_part)
{
  return c == create_cookie_initiator(secret, secret_size, initiator,
      responder, initiator_part);
}

inline bool
validate_cookie(cookie const & c,
    std::byte const * secret, std::size_t secret_size,
    peerid_wrapper const & initiator,
    peerid_wrapper const & responder,
    channelid const & id)
{
  return c == create_cookie_responder(secret, secret_size, initiator,
      responder, id);
}


} // namespace channeler


#endif // guard
