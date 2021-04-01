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
#include <channeler/cookie.h>

#include <liberate/serialization/integer.h>
#include <liberate/serialization/varint.h>

namespace channeler {


namespace {

inline std::size_t
serialize_header(std::byte * buf, std::size_t max, message_type type,
    std::size_t payload_size = 0)
{
  std::size_t total = 0;

  // We know the message type, and need to serialize it.
  auto tmp = static_cast<liberate::types::varint>(type);
  auto used = liberate::serialization::serialize_varint(
      buf + total,
      max - total,
      tmp);
  if (used <= 0) {
    return 0;
  }
  total += used;

  // Also serialize size, if it is given.
  if (payload_size > 0) {
    tmp = static_cast<liberate::types::varint>(payload_size);
    used = liberate::serialization::serialize_varint(
        buf + total,
        max - total,
        tmp);
    if (used <= 0) {
      return 0;
    }
    total += used;
  }

  return total;
}


inline std::size_t
serialize_header(std::byte * buf, std::size_t max, message const & msg)
{
  return serialize_header(buf, max, msg.type);
}


} // anonymous namespace

ssize_t
message_payload_size(message_type_base type)
{
  // TODO extending this is error-prone, which is also a good reason for
  //      registering known message types
  //      https://gitlab.com/interpeer/channeler/-/issues/1
  switch (type) {
    case MSG_CHANNEL_NEW:
      // - channelid.initiator
      // - cookie1
      return sizeof(channelid::half_type) + sizeof(cookie_serialize);

    case MSG_CHANNEL_ACKNOWLEDGE:
      // - channelid.full
      // - cookie2
      return sizeof(channelid::full_type) + sizeof(cookie_serialize) * 2;

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


/**
 * message
 */
message::message(std::byte const * buf, std::size_t max,
    bool parse_now /* = true */)
  : buffer{buf}
  , input_size{max}
{
  if (parse_now) {
    auto err = parse();
    if (err.first != ERR_SUCCESS) {
      throw exception{err.first, err.second};
    }
  }
}



std::pair<error_t, std::string>
message::parse()
{
  // Extract the type, and ensure it is known.
  liberate::types::varint tmp;
  auto used = liberate::serialization::deserialize_varint(tmp, buffer, input_size);
  if (!used) {
    return {ERR_DECODE, "Could not decode message type"};
  }

  message_type_base the_type = static_cast<message_type_base>(tmp);
  ssize_t fixed_size = message_payload_size(the_type);
  if (fixed_size == -2) {
    return {ERR_INVALID_MESSAGE_TYPE, "The message type encoded in the buffer is unsupported."};
  }
  *const_cast<message_type *>(&type) = static_cast<message_type>(the_type);

  // For fixed message types, we can just use the known message size.
  if (fixed_size >= 0) {
    // The size can be applied to the buffer already.
    if (static_cast<std::size_t>(fixed_size) > input_size) {
      return {ERR_INSUFFICIENT_BUFFER_SIZE, "The message type requires a bigger input buffer."};
    }
    *const_cast<std::size_t *>(&buffer_size) = fixed_size + used;
    *const_cast<std::byte const **>(&payload) = buffer + used;
    *const_cast<std::size_t *>(&payload_size) = buffer_size - used;
  }
  else {
    // Variable length messages have the payload size included as a varint
    auto used2 = liberate::serialization::deserialize_varint(tmp, buffer + used, input_size - used);
    if (!used2) {
      return {ERR_DECODE, "Could not decode message length"};
    }
    *const_cast<std::size_t *>(&payload_size) = static_cast<std::size_t>(tmp);
    *const_cast<std::size_t *>(&buffer_size) = payload_size + used + used2;
    *const_cast<std::byte const **>(&payload) = buffer + used + used2;
  }

  // Payload processing is part of subtypes, and not happening here.

  return {ERR_SUCCESS, {}};
}



std::size_t
message::serialized_size() const
{
  auto t = static_cast<::liberate::types::varint>(type);
  std::size_t result = ::liberate::serialization::serialized_size(t);

  auto pl_size = message_payload_size(type);
  if (pl_size> 0) {
    result += pl_size;
  }
  else if (pl_size == -1) {
    t = static_cast<::liberate::types::varint>(payload_size);
    result += ::liberate::serialization::serialized_size(t);
    result += payload_size;
  }
  else if (pl_size < -2) {
    // Error, unknown message type
    return 0;
  }

  return result;
}



/**
 * message_data
 */
std::unique_ptr<message>
message_data::extract_features(message const & wrap)
{
  auto ptr = new message_data{wrap};
  return std::unique_ptr<message_data>(ptr);
}



message_data::message_data(message const & wrap)
  : message{wrap}
{
}


message_data::message_data(std::vector<std::byte> && data)
  : message{nullptr, 0, false}
  , owned_buffer{std::move(data)}
{
  *const_cast<std::byte const **>(&buffer) = &owned_buffer[0];
  *const_cast<std::size_t *>(&input_size) = owned_buffer.size();

  auto err = parse();
  if (err.first != ERR_SUCCESS) {
    throw exception{err.first, err.second};
  }
}


std::unique_ptr<message>
message_data::create(std::byte const * buf, std::size_t size)
{
  if (!buf || !size) {
    return {};
  }

  // We need to understand the length of the serialized message header. We know
  // it is at maximum two varints in size, so we'll serialize this first.
  std::vector<std::byte> result;
  std::size_t reserved = liberate::serialization::VARINT_MAX_BUFSIZE * 2;
  result.resize(size + reserved);
  auto used = serialize_header(&result[0], reserved,
      MSG_DATA, size);

  result.resize(size + used);
  memcpy(&result[0] + used, buf, size);

  auto ptr = new message_data{std::move(result)};
  return std::unique_ptr<message>(ptr);
}



std::unique_ptr<message>
message_data::create(std::vector<std::byte> & data)
{
  if (data.empty()) {
    return {};
  }

  // We need to understand the length of the serialized message header. We know
  // it is at maximum two varints in size, so we'll serialize this first.
  std::vector<std::byte> result;
  std::size_t reserved = liberate::serialization::VARINT_MAX_BUFSIZE * 2;
  result.resize(data.size() + reserved);
  auto used = serialize_header(&result[0], reserved,
      MSG_DATA, data.size());

  result.resize(used);
  result.insert(result.end(),
      std::make_move_iterator(data.begin()),
      std::make_move_iterator(data.end()));

  auto ptr = new message_data{std::move(result)};
  return std::unique_ptr<message>(ptr);
}



std::size_t
message_data::serialize(std::byte * out, std::size_t max,
    message_data const & msg)
{
  // XXX This assumes that the buffer_size is set. This is done when parsing
  //     and when creating from data, see create() above.
  if (msg.serialized_size() > max) {
    return 0;
  }
  ::memcpy(out, msg.buffer, msg.buffer_size);
  return msg.buffer_size;
}




/**
 * message_channel_new
 */
std::unique_ptr<message>
message_channel_new::extract_features(message const & wrap)
{
  auto * ptr = new message_channel_new{wrap};
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

  return std::unique_ptr<message>(ptr);
}



message_channel_new::message_channel_new(message const & wrap)
  : message{wrap}
{
}



std::size_t
message_channel_new::serialize(std::byte * out, std::size_t max,
    message_channel_new const & msg)
{
  // We know the buffer size, as it's fixed.
  if (msg.serialized_size() > max) {
    return 0;
  }

  std::size_t remaining = msg.serialized_size();
  std::byte * offset = out;

  // Serialize message header
  auto used = serialize_header(offset, remaining, msg);
  if (used <= 0) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the half id.
  used = liberate::serialization::serialize_int(offset, remaining,
      msg.initiator_part);
  if (used != sizeof(msg.initiator_part)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the cookie
  cookie_serialize tmp = msg.cookie1;
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp);
  if (used != sizeof(tmp)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  if (remaining != 0) {
    return 0;
  }
  return (offset - out);
}



/**
 * message_channel_acknowledge
 */
std::unique_ptr<message>
message_channel_acknowledge::extract_features(message const & wrap)
{
  auto * ptr = new message_channel_acknowledge{wrap};
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

  // And cookie1
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

  // cookie2
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

  return std::unique_ptr<message>(ptr);
}



message_channel_acknowledge::message_channel_acknowledge(message const & wrap)
  : message{wrap}
{
}



std::size_t
message_channel_acknowledge::serialize(std::byte * out, std::size_t max,
    message_channel_acknowledge const & msg)
{
  // We know the buffer size, as it's fixed.
  if (msg.serialized_size() > max) {
    return 0;
  }

  std::size_t remaining = msg.serialized_size();
  std::byte * offset = out;

  // Serialize message header
  auto used = serialize_header(offset, remaining, msg);
  if (used <= 0) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the full id.
  used = liberate::serialization::serialize_int(offset, remaining,
      msg.id.full);
  if (used != sizeof(msg.id.full)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize cookie1
  cookie_serialize tmp = msg.cookie1;
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp);
  if (used != sizeof(tmp)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize cookie2
  tmp = msg.cookie2;
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp);
  if (used != sizeof(tmp)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  if (remaining != 0) {
    return 0;
  }
  return (offset - out);
}



/**
 * message_channel_finalize
 */
std::unique_ptr<message>
message_channel_finalize::extract_features(message const & wrap)
{
  auto * ptr = new message_channel_finalize{wrap};
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

  return std::unique_ptr<message>(ptr);
}



message_channel_finalize::message_channel_finalize(message const & wrap)
  : message{wrap}
{
}



std::size_t
message_channel_finalize::serialize(std::byte * out, std::size_t max,
    message_channel_finalize const & msg)
{
  // We know the buffer size, as it's fixed.
  if (msg.serialized_size() > max) {
    return 0;
  }

  std::size_t remaining = msg.serialized_size();
  std::byte * offset = out;

  // Serialize message header
  auto used = serialize_header(offset, remaining, msg);
  if (used <= 0) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the full id.
  used = liberate::serialization::serialize_int(offset, remaining,
      msg.id.full);
  if (used != sizeof(msg.id.full)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the cookie
  cookie_serialize tmp = msg.cookie2;
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp);
  if (used != sizeof(tmp)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Capabilities
  capability_bits_t tmp2 = msg.capabilities.to_ullong();
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp2);
  if (used != sizeof(tmp2)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  if (remaining != 0) {
    return 0;
  }
  return (offset - out);
}



/**
 * message_channel_cookie
 */
std::unique_ptr<message>
message_channel_cookie::extract_features(message const & wrap)
{
  auto * ptr = new message_channel_cookie{wrap};
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

  return std::unique_ptr<message>(ptr);
}



message_channel_cookie::message_channel_cookie(message const & wrap)
  : message{wrap}
{
}



std::size_t
message_channel_cookie::serialize(std::byte * out, std::size_t max,
    message_channel_cookie const & msg)
{
  // We know the buffer size, as it's fixed.
  if (msg.serialized_size() > max) {
    return 0;
  }

  std::size_t remaining = msg.serialized_size();
  std::byte * offset = out;

  // Serialize message header
  auto used = serialize_header(offset, remaining, msg);
  if (used <= 0) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Serialize the cookie
  cookie_serialize tmp = msg.either_cookie;
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp);
  if (used != sizeof(tmp)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  // Capabilities
  capability_bits_t tmp2 = msg.capabilities.to_ullong();
  used = liberate::serialization::serialize_int(offset, remaining,
      tmp2);
  if (used != sizeof(tmp2)) {
    return 0;
  }
  offset += used;
  remaining -= used;

  if (remaining != 0) {
    return 0;
  }
  return (offset - out);
}



/**
 * Parse/serialize
 */
std::unique_ptr<message>
extract_message_features(message const & msg)
{
  // TODO *especially* this screams for some kind of registry for factory
  //      functions per type.
  //      https://gitlab.com/interpeer/channeler/-/issues/1

  switch (msg.type) {
    case MSG_CHANNEL_NEW:
      return message_channel_new::extract_features(msg);

    case MSG_CHANNEL_ACKNOWLEDGE:
      return message_channel_acknowledge::extract_features(msg);

    case MSG_CHANNEL_FINALIZE:
      return message_channel_finalize::extract_features(msg);

    case MSG_CHANNEL_COOKIE:
      return message_channel_cookie::extract_features(msg);

    case MSG_DATA:
      // Must make copy
      return message_data::extract_features(msg);

    default:
      break;
  }

  // Unable to parse anything
  return {};
}



std::size_t
serialize_message(std::byte * output, std::size_t max,
    std::unique_ptr<message> const & msg)
{
  // TODO *especially* this screams for some kind of registry for factory
  //      functions per type.
  //      https://gitlab.com/interpeer/channeler/-/issues/1

  switch (msg->type) {
    case MSG_CHANNEL_NEW:
      return message_channel_new::serialize(output, max,
          *reinterpret_cast<message_channel_new const *>(msg.get())
      );

    case MSG_CHANNEL_ACKNOWLEDGE:
      return message_channel_acknowledge::serialize(output, max,
          *reinterpret_cast<message_channel_acknowledge const *>(msg.get())
      );

    case MSG_CHANNEL_FINALIZE:
      return message_channel_finalize::serialize(output, max,
          *reinterpret_cast<message_channel_finalize const *>(msg.get())
      );

    case MSG_CHANNEL_COOKIE:
      return message_channel_cookie::serialize(output, max,
          *reinterpret_cast<message_channel_cookie const *>(msg.get())
      );

    case MSG_DATA:
      return message_data::serialize(output, max,
          *reinterpret_cast<message_data const *>(msg.get())
      );

    default:
      break;
  }

  // Unable to produce anything
  return 0;
}

} // namespace channeler
