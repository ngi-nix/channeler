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
#ifndef CHANNELER_MESSAGE_H
#define CHANNELER_MESSAGE_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <liberate/types.h>
#include <liberate/serialization/varint.h>

namespace channeler {

/**
 * Messages have a type.
 *
 * Note that the message_type_base is 16 bits, but the type is encoded as a
 * variable length integer.
 *
 * TODO in future, it should be possible to extend the protocol by adding
 *      to a set of registered messages in a namespace. We'll skip this for
 *      the time being.
 */
using message_type_base = uint16_t;
enum message_type : message_type_base
{
  MSG_UNKNOWN = 0,

  // Channel negotiation
  MSG_CHANNEL_NEW = 10,
  MSG_CHANNEL_ACKNOWLEDGE,
  MSG_CHANNEL_FINALIZE,
  MSG_CHANNEL_COOKIE,

  // Transmission related
  MSG_DATA = 20,

  // TODO
  // MSG_DATA_PRORESS,
  // MSG_DATA_RECEIVE_WINDOW,
  // MSG_START_ENCRYPTION
};


/**
 * Analogous to packet_wrapper and packet, we define the message_wrapper class
 * as just taking a buffer and providing an interface for messages. At the same
 * time, the message class also manages an internal buffer.
 *
 * Messages also subdivide into those of fixed and of variable size. This is
 * based on their type. It introduces the problem that for messages that
 * manage their buffer, we can grow and shrink the buffer e.g. during
 * construction of messages.
 *
 * To simplify things, we do *not* allow adjusting the buffer size for
 * packet_wrapper.
 *
 * Lastly, it's up to the caller to construct message classes that give easier
 * access to the payload from a message_base. All we are concerned with here
 * are the delineation between one message and another in an input buffer.
 */
struct message_base
{
  // Message metadata
  message_type const  type;

  // Buffer
  std::byte *         buffer;
  std::size_t         buffer_size;

  // Payload
  std::byte *         payload;
  std::size_t         payload_size;
};

struct message_wrapper : public message_base
{
  message_wrapper(std::byte * buf, std::size_t max, bool validate_now = true);

  std::pair<error_t, std::string>
  validate(std::size_t max);
};


} // namespace channeler

#endif // guard
