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

#include <iostream> // FIXME

#include <channeler/message.h>

#include <channeler/channelid.h>
#include <channeler/capabilities.h>
#include <channeler/cookie.h>

#include <liberate/serialization/integer.h>
#include <liberate/serialization/varint.h>

namespace channeler {

namespace {

inline ssize_t
payload_size_of_message(message_type_base const type)
{
  // TODO extending this is error-prone, which is also a good reason for
  //      registering known message types
  switch (type) {
    case MSG_CHANNEL_NEW:
      // - channelid.initiator
      // - cookie1
      return sizeof(channelid::half_type) + sizeof(cookie_serialize);

    case MSG_CHANNEL_ACKNOWLEDGE:
      // - channelid.full
      // - cookie2
      return sizeof(channelid::full_type) + sizeof(cookie_serialize);

    case MSG_CHANNEL_FINALIZE:
      // - channelid.full
      // - cookie2
      // - capability bits
      return sizeof(channelid::full_type) + sizeof(cookie_serialize)
        + sizeof(capability_bits_t);

    case MSG_CHANNEL_COOKIE:
      // - channelid.full // In header
      // - either cookie
      // - capability bits
      return sizeof(cookie_serialize) + sizeof(capability_bits_t);

    case MSG_DATA:
      return -1;

    default:
      return -2;
  }
}

} // anonymous namespace


/**
 * message_wrapper
 */
message_wrapper::message_wrapper(std::byte const * buf, std::size_t max,
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
    if (static_cast<std::size_t>(fixed_size) > max) {
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



/**
 * message_wrapper_channel_new
 */
std::unique_ptr<message_wrapper>
message_wrapper_channel_new::from_wrapper(message_wrapper const & wrap)
{
  auto * ptr = new message_wrapper_channel_new{wrap};
  std::byte const * offset = ptr->payload;
  std::size_t size = ptr->payload_size;

  // First comes the partial channel id
  auto used = liberate::serialization::deserialize_int(ptr->initiator_part,
      offset, size);
  if (used != sizeof(ptr->initiator_part)) {
    delete ptr;
    return {};
  }
  offset += used;
  size -= used;

  // And the cookie
  cookie_serialize s;
  used = liberate::serialization::deserialize_int(s,
      offset, size);
  if (used != sizeof(s)) {
    delete ptr;
    return {};
  }
  ptr->cookie1 = s;

  offset += used;
  size -= used;

  if (size > 0) {
    // We didn't consume the entire payload
    delete ptr;
    return {};
  }

  return std::unique_ptr<message_wrapper>(ptr);
}


message_wrapper_channel_new::message_wrapper_channel_new(message_wrapper const & wrap)
  : message_wrapper{wrap}
{
}



/**
 * message_wrapper_channel_acknowledge
 */
std::unique_ptr<message_wrapper>
message_wrapper_channel_acknowledge::from_wrapper(message_wrapper const & wrap)
{
  auto * ptr = new message_wrapper_channel_acknowledge{wrap};
  std::byte const * offset = ptr->payload;
  std::size_t size = ptr->payload_size;

  // First comes the channel id
  auto used = liberate::serialization::deserialize_int(ptr->id.full,
      offset, size);
  if (used != sizeof(ptr->id.full)) {
    delete ptr;
    return {};
  }
  offset += used;
  size -= used;

  // And the cookie
  cookie_serialize s;
  used = liberate::serialization::deserialize_int(s,
      offset, size);
  if (used != sizeof(s)) {
    delete ptr;
    return {};
  }
  ptr->cookie2 = s;

  offset += used;
  size -= used;

  if (size > 0) {
    // We didn't consume the entire payload
    delete ptr;
    return {};
  }

  return std::unique_ptr<message_wrapper>(ptr);
}


message_wrapper_channel_acknowledge::message_wrapper_channel_acknowledge(message_wrapper const & wrap)
  : message_wrapper{wrap}
{
}



/**
 * message_wrapper_channel_finalize
 */
std::unique_ptr<message_wrapper>
message_wrapper_channel_finalize::from_wrapper(message_wrapper const & wrap)
{
  auto * ptr = new message_wrapper_channel_finalize{wrap};
  std::byte const * offset = ptr->payload;
  std::size_t size = ptr->payload_size;

  // First comes the channel id
  auto used = liberate::serialization::deserialize_int(ptr->id.full,
      offset, size);
  if (used != sizeof(ptr->id.full)) {
    delete ptr;
    return {};
  }
  offset += used;
  size -= used;

  // And the cookie
  cookie_serialize s;
  used = liberate::serialization::deserialize_int(s,
      offset, size);
  if (used != sizeof(s)) {
    delete ptr;
    return {};
  }
  ptr->cookie2 = s;

  offset += used;
  size -= used;

  // Also the capabilities
  capability_bits_t bits;
  used = liberate::serialization::deserialize_int(bits,
      offset, size);
  if (used != sizeof(bits)) {
    delete ptr;
    return {};
  }
  ptr->capabilities = bits;

  offset += used;
  size -= used;


  if (size > 0) {
    // We didn't consume the entire payload
    delete ptr;
    return {};
  }

  return std::unique_ptr<message_wrapper>(ptr);
}


message_wrapper_channel_finalize::message_wrapper_channel_finalize(message_wrapper const & wrap)
  : message_wrapper{wrap}
{
}




/**
 * message_wrapper_channel_cookie
 */
std::unique_ptr<message_wrapper>
message_wrapper_channel_cookie::from_wrapper(message_wrapper const & wrap)
{
  auto * ptr = new message_wrapper_channel_cookie{wrap};
  std::byte const * offset = ptr->payload;
  std::size_t size = ptr->payload_size;

  // The cookie...
  cookie_serialize s;
  auto used = liberate::serialization::deserialize_int(s,
      offset, size);
  if (used != sizeof(s)) {
    delete ptr;
    return {};
  }
  ptr->either_cookie = s;

  offset += used;
  size -= used;

  // Also the capabilities
  capability_bits_t bits;
  used = liberate::serialization::deserialize_int(bits,
      offset, size);
  if (used != sizeof(bits)) {
    delete ptr;
    return {};
  }
  ptr->capabilities = bits;

  offset += used;
  size -= used;


  if (size > 0) {
    // We didn't consume the entire payload
    delete ptr;
    return {};
  }

  return std::unique_ptr<message_wrapper>(ptr);
}


message_wrapper_channel_cookie::message_wrapper_channel_cookie(message_wrapper const & wrap)
  : message_wrapper{wrap}
{
}


/**
 * parse_message
 */
std::unique_ptr<message_wrapper>
parse_message(std::byte const * buffer, std::size_t size)
{
  message_wrapper wrap{buffer, size, false};
  auto err = wrap.validate(size);
  if (ERR_SUCCESS != err.first) {
    return {};
  }

  // TODO *especially* this screams for some kind of registry for factory
  //      functions per type.

  switch (wrap.type) {
    case MSG_CHANNEL_NEW:
      return message_wrapper_channel_new::from_wrapper(wrap);

    case MSG_CHANNEL_ACKNOWLEDGE:
      return message_wrapper_channel_acknowledge::from_wrapper(wrap);

    case MSG_CHANNEL_FINALIZE:
      return message_wrapper_channel_finalize::from_wrapper(wrap);

    case MSG_CHANNEL_COOKIE:
      return message_wrapper_channel_cookie::from_wrapper(wrap);

    case MSG_DATA:
      // Must make copy
      return std::make_unique<message_wrapper>(wrap);

    default:
      break;
  }

  // Unable to parse anything
  return {};
}

} // namespace channeler
