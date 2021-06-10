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

#include <vector>
#include <ostream>

#include <liberate/types.h>
#include <liberate/serialization/varint.h>

#include <channeler/channelid.h>
#include <channeler/capabilities.h>
#include <channeler/cookie.h>

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
 *      https://gitlab.com/interpeer/channeler/-/issues/1
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
  // MSG_DATA_PROGRESS,
  // MSG_DATA_RECEIVE_WINDOW,
  // https://gitlab.com/interpeer/channeler/-/issues/2
  //
  // TODO
  // MSG_START_ENCRYPTION
  // https://gitlab.com/interpeer/channeler/-/issues/3
};

inline std::ostream &
operator<<(std::ostream & os, message_type type)
{
  os << static_cast<message_type_base>(type);
  return os;
}


/**
 * Analogous to packet_wrapper and packet, we define the message class
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
 * access to the payload from a message. All we are concerned with here
 * are the delineation between one message and another in an input buffer.
 */
struct message
{
  // Message metadata
  message_type const      type = MSG_UNKNOWN;

  // Buffer
  byte const * const      buffer = nullptr;
  std::size_t const       input_size = 0;
  std::size_t const       buffer_size = 0;

  // Payload
  byte const * const      payload = nullptr;
  std::size_t const       payload_size = 0;

  /**
   * Constructor parses the input buffer, and fills the basic fields
   * above.
   */
  message(byte const * buf, std::size_t max, bool parse_now = true);

  virtual ~message() = default;

  /**
   * For delayed parsing/validation
   */
  std::pair<error_t, std::string>
  parse();

  /**
   * Serialized size of the message. This is type dependent.
   */
  std::size_t serialized_size() const;

  // TODO
  // std::size_t serialized_size() const
  // - serialied size of message type (calculate in liberate)
  // - either hardcoded fixed size
  //  - or payload_size-based (msg_data)

protected:
  inline message(message_type t)
    : type{t}
  {
  }
};



/**
 * For parsing purposes, we need to know the payload size of message types.
 * The result can either be a byte value, -1 for a variable sized message, or
 * -2 if the type is unknown.
 *
 * Variable sized messages have a payload size following the message type in
 * their serialization.
 *
 * TODO this will become part of a registry later
 */
ssize_t message_payload_size(message_type_base type);


/**
 * For extracting more message data, we use a factory approach that takes a
 * message and constructs a new message subtype from it. That has to
 * then be returned as a unique_ptr for handling owneship well.
 *
 * The parse_message() function takes this from a raw input buffer, and returns
 * an empty pointer on failure.
 *
 * TODO this will become part of a registry later
 */
std::unique_ptr<message>
extract_message_features(message const & msg);

inline std::unique_ptr<message>
parse_message(byte const * buffer, std::size_t size)
{
  message msg{buffer, size, false};
  auto err = msg.parse();
  if (ERR_SUCCESS != err.first) {
    return {};
  }

  return extract_message_features(msg);
}


/**
 * For assembling messages, it gets very complicated if we want to use pre-
 * allocated memory already associated with e.g. a packet buffer, because we
 * don't necessarily know how to pack messages together into a packet when
 * the message is being constructed. So we return a byte vector here with
 * the message's basic data.
 *
 * TODO also this will move into the registry
 */
std::size_t
serialize_message(byte * output, std::size_t max_size,
    std::unique_ptr<message> const & msg);



/**
 * Messages
 */
struct message_channel_new
  : public message
{
  channelid::half_type  initiator_part = DEFAULT_CHANNELID.initiator;
  cookie                cookie1 = {};

  inline message_channel_new(channelid::half_type const & initiator,
      cookie const & _cookie1)
    : message{MSG_CHANNEL_NEW}
    , initiator_part{initiator}
    , cookie1{_cookie1}
  {
  }

  static std::unique_ptr<message>
  extract_features(message const & wrap);

  static std::size_t
  serialize(byte * out, std::size_t max, message_channel_new const & msg);

private:
  explicit message_channel_new(message const & wrap);
};


struct message_channel_acknowledge
  : public message
{
  channelid       id = DEFAULT_CHANNELID;
  cookie          cookie1 = {};
  cookie          cookie2 = {};

  inline message_channel_acknowledge(channelid const & _id,
      cookie const & _cookie1,
      cookie const & _cookie2)
    : message{MSG_CHANNEL_ACKNOWLEDGE}
    , id{_id}
    , cookie1{_cookie1}
    , cookie2{_cookie2}
  {
  }

  static std::unique_ptr<message>
  extract_features(message const & wrap);

  static std::size_t
  serialize(byte * out, std::size_t max, message_channel_acknowledge const & msg);

private:
  explicit message_channel_acknowledge(message const & wrap);
};


struct message_channel_finalize
  : public message
{
  channelid       id = DEFAULT_CHANNELID;
  cookie          cookie2 = {};
  capabilities_t  capabilities = {};

  inline message_channel_finalize(channelid const & _id,
      cookie const & _cookie2,
      capabilities_t const & _capabilities)
    : message{MSG_CHANNEL_FINALIZE}
    , id{_id}
    , cookie2{_cookie2}
    , capabilities{_capabilities}
  {
  }



  static std::unique_ptr<message>
  extract_features(message const & wrap);

  static std::size_t
  serialize(byte * out, std::size_t max, message_channel_finalize const & msg);

private:
  explicit message_channel_finalize(message const & wrap);
};


struct message_channel_cookie
  : public message
{
  cookie          either_cookie = {};
  capabilities_t  capabilities = {};

  static std::unique_ptr<message>
  extract_features(message const & wrap);

  static std::size_t
  serialize(byte * out, std::size_t max, message_channel_cookie const & msg);

private:
  explicit message_channel_cookie(message const & wrap);
};


struct message_data
  : public message
{
  // TODO
  // - maybe make constructors in other messages private and offer
  //   the same kind of interface
  // - deal with ownership somehow

  /**
   * Create a data message from a *data* buffer. This data buffer does not
   * include the type or size.
   *
   * This *must* create messages that take ownership of data, so that is
   * what we do here. That means that the vector version moves the data.
   */
  static std::unique_ptr<message>
  create(byte const * buf, std::size_t max);

  static std::unique_ptr<message>
  create(std::vector<byte> & data);

  static std::unique_ptr<message>
  extract_features(message const & wrap);

  static std::size_t
  serialize(byte * out, std::size_t max, message_data const & msg);

private:
  explicit message_data(message const & wrap);
  explicit message_data(std::vector<byte> && data);

  std::vector<byte>  owned_buffer;
};




/**
 * Provide an iterator interface for messages.
 */
struct messages
{
  using value_type = std::unique_ptr<message>;

  struct iterator : public std::iterator<
    std::input_iterator_tag,   // iterator_category
    value_type,                // value_type
    long,                      // difference_type
    value_type const *,        // pointer
    value_type const &         // reference
  >
  {
    inline value_type operator*() const
    {
      if (m_offset >= m_messages.m_size) {
        return {};
      }
      auto msg = parse_message(
        m_messages.m_buffer + m_offset,
        m_messages.m_size - m_offset
      );
      if (msg) {
        return msg;
      }
      return {};
    }


    inline iterator & operator++()
    {
      // FIXME:
      // - remaining() also needs a tmp message
      // - better:
      //    - parse header separately from message
      //    - keep it as a iterator member
      //    - use the parsed size there instead of a temp
      //    - then on dereferencing create a new message based on the
      //      pre-parsed header
      // https://gitlab.com/interpeer/channeler/-/issues/4
      auto msg = parse_message(
          m_messages.m_buffer + m_offset,
          m_messages.m_size - m_offset
      );
      if (msg) {
        m_offset += msg->buffer_size;
      }
      return *this;
    }

    inline iterator operator++(int) // postfix
    {
      iterator ret{*this};
      ++ret;
      return ret;
    }


    inline std::size_t remaining() const
    {
      // FIXME see above
      auto msg = parse_message(
          m_messages.m_buffer + m_offset,
          m_messages.m_size - m_offset
      );
      std::size_t remain = m_messages.m_size - m_offset;
      if (msg) {
        remain -= msg->buffer_size;
      }

      return remain;
    }


    inline bool operator==(iterator const & other) const
    {
      return (**this) == (*other);
    }

    inline bool operator!=(iterator const & other) const
    {
      return (**this) != (*other);
    }

  private:
    friend struct messages;

    inline iterator(messages const & msg, std::size_t offset)
      : m_messages{msg}
      , m_offset{offset}
    {
    }

    messages const &  m_messages;
    std::size_t       m_offset;
  };

  using const_iterator = iterator const;

  inline messages(byte const * buffer, std::size_t size)
    : m_buffer{buffer}
    , m_size{size}
  {
  }

  inline iterator begin()
  {
    return {*this, 0};
  }

  inline const_iterator begin() const
  {
    return {*this, 0};
  }

  inline iterator end()
  {
    return {*this, m_size};
  }

  inline const_iterator end() const
  {
    return {*this, m_size};
  }

  inline bool operator==(messages const & other) const
  {
    return m_buffer == other.m_buffer &&
      m_size == other.m_size;
  }

  inline bool operator!=(messages const & other) const
  {
    return m_buffer != other.m_buffer ||
      m_size != other.m_size;
  }


  byte const *      m_buffer;
  std::size_t       m_size;
};


} // namespace channeler

#endif // guard
