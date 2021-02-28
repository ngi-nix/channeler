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

#include <iostream> // FIXME

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
  message_type const  type = MSG_UNKNOWN;

  // Buffer
  std::byte const *   buffer = nullptr;
  std::size_t         buffer_size = 0;

  // Payload
  std::byte const *   payload = nullptr;
  std::size_t         payload_size = 0;
};

struct message_wrapper : public message_base
{
  message_wrapper(std::byte const * buf, std::size_t max, bool validate_now = true);

  std::pair<error_t, std::string>
  validate(std::size_t max);
};



/**
 * Messages
 */
struct message_wrapper_channel_new
  : public message_wrapper
{
  channelid::half_type  initiator_part = DEFAULT_CHANNELID.initiator;
  cookie                cookie1 = {};

  static std::unique_ptr<message_wrapper>
  from_wrapper(message_wrapper const & wrap);
private:
  explicit message_wrapper_channel_new(message_wrapper const & wrap);
};


struct message_wrapper_channel_acknowledge
  : public message_wrapper
{
  channelid       id = DEFAULT_CHANNELID;
  cookie          cookie2 = {};

  static std::unique_ptr<message_wrapper>
  from_wrapper(message_wrapper const & wrap);
private:
  explicit message_wrapper_channel_acknowledge(message_wrapper const & wrap);
};


struct message_wrapper_channel_finalize
  : public message_wrapper
{
  channelid       id = DEFAULT_CHANNELID;
  cookie          cookie2 = {};
  capabilities_t  capabilities = {};

  static std::unique_ptr<message_wrapper>
  from_wrapper(message_wrapper const & wrap);
private:
  explicit message_wrapper_channel_finalize(message_wrapper const & wrap);
};


struct message_wrapper_channel_cookie
  : public message_wrapper
{
  cookie          either_cookie = {};
  capabilities_t  capabilities = {};

  static std::unique_ptr<message_wrapper>
  from_wrapper(message_wrapper const & wrap);
private:
  explicit message_wrapper_channel_cookie(message_wrapper const & wrap);
};


struct message_data : public message_wrapper {};



/**
 * For simplicity's sake, we provide a factory function that returns a
 * message_wrapper pointer. The message's buffer size represents the number
 * of bytes used from the input buffer.
 */
std::unique_ptr<message_wrapper>
parse_message(std::byte const * buffer, std::size_t size);



/**
 * Provide an iterator interface for messages.
 */
struct messages
{
  using value_type = std::unique_ptr<message_wrapper>;

  struct iterator
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

    inline value_type operator->() const
    {
      return this->operator*();
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
      //
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

  inline messages(std::byte const * buffer, std::size_t size)
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


  std::byte const * m_buffer;
  std::size_t       m_size;
};


} // namespace channeler

#endif // guard
