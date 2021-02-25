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

#include <channeler/message.h>

#include <channeler/channelid.h>
#include <channeler/capabilities.h>

#include <liberate/serialization/integer.h>
#include <liberate/serialization/varint.h>

namespace channeler {

namespace {

  // FIXME
using cookie = uint32_t;

inline ssize_t
payload_size_of_message(message_type_base const type)
{
  // TODO extending this is error-prone, which is also a good reason for
  //      registering known message types
  switch (type) {
    case MSG_CHANNEL_NEW:
      // - channelid.initiator
      // - cookie1
      return sizeof(channelid::half_type) + sizeof(cookie);

    case MSG_CHANNEL_ACKNOWLEDGE:
      // - channelid.full
      // - cookie2
      return sizeof(channelid::full_type) + sizeof(cookie);

    case MSG_CHANNEL_FINALIZE:
      // - channelid.full
      // - cookie2
      // - capability bits
      return sizeof(channelid::full_type) + sizeof(cookie)
        + sizeof(capability_bits_t);

    case MSG_CHANNEL_COOKIE:
      // - channelid.full
      // - either cookie
      // - capability bits
      return sizeof(channelid::full_type) + sizeof(cookie)
        + sizeof(capability_bits_t);

    case MSG_DATA:
      return -1;

    default:
      return -2;
  }
}

} // anonymous namespace

message_wrapper::message_wrapper(std::byte * buf, std::size_t max,
    bool validate_now /* = true */)
  : message_base{MSG_UNKNOWN, buf, 0, nullptr, 0}
{
  if (validate_now) {
    auto err = validate(max);
    if (err.first != ERR_SUCCESS) {
      throw exception{err.first, err.second};
    }
  }
}



std::pair<error_t, std::string>
message_wrapper::validate(std::size_t max)
{
  // Extract the type, and ensure it is known.
  liberate::types::varint tmp;
  auto used = liberate::serialization::deserialize_varint(tmp, buffer, max);
  if (!used) {
    return {ERR_DECODE, "Could not decode message type"};
  }

  message_type_base the_type = static_cast<message_type_base>(tmp);
  ssize_t fixed_size = payload_size_of_message(the_type);
  if (fixed_size == -2) {
    return {ERR_INVALID_MESSAGE_TYPE, "The message type encoded in the buffer is unsupported."};
  }
  *const_cast<message_type *>(&type) = static_cast<message_type>(the_type);

  // For fixed message types, we can just use the known message size.
  if (fixed_size >= 0) {
    // The size can be applied to the buffer already.
    if (fixed_size > max) {
      return {ERR_INSUFFICIENT_BUFFER_SIZE, "The message type requires a bigger input buffer."};
    }
    buffer_size = fixed_size + used;
    payload = buffer + used;
    payload_size = buffer_size - used;
  }
  else {
    // Variable length messages have the size included as a varint
    auto used2 = liberate::serialization::deserialize_varint(tmp, buffer + used, max - used);
    if (!used2) {
      return {ERR_DECODE, "Could not decode message length"};
    }
    buffer_size = static_cast<std::size_t>(tmp);
    payload = buffer + used + used2;
    payload_size = buffer_size - used - used2;
  }

  // Payload processing is part of subtypes, and not happening here.

  return {ERR_SUCCESS, {}};
}



} // namespace channeler
