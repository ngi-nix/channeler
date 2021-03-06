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

#include <channeler/message.h>

#include <gtest/gtest.h>

#include "../messages.h"

using namespace test;

namespace {


inline void
assert_message(std::vector<channeler::byte> const & buf,
    channeler::message_type type,
    std::size_t type_bytes,
    std::size_t length_bytes)
{
  ASSERT_NO_THROW((channeler::message{buf.data(), buf.size()}));

  // Validate later
  channeler::message msg{buf.data(), buf.size(), false};
  ASSERT_EQ(msg.buffer, buf.data());
  ASSERT_EQ(msg.input_size, buf.size());
  auto err = msg.parse();
  ASSERT_EQ(err.first, channeler::ERR_SUCCESS);

  // Payload should start one byte after the buffer.
  ASSERT_EQ(msg.buffer + type_bytes + length_bytes, msg.payload);
  ASSERT_EQ(msg.payload_size + type_bytes + length_bytes, msg.buffer_size);
  ASSERT_EQ(msg.buffer, buf.data());
  ASSERT_EQ(msg.buffer_size, buf.size());

  // Assert type
  ASSERT_EQ(msg.type, type);
}



inline void
assert_single_byte_type_variable_size_message(std::vector<channeler::byte> const & buf,
    channeler::message_type type, std::size_t length_bytes)
{
  assert_message(buf, type, 1, length_bytes);
}



inline void
assert_single_byte_type_fixed_size_message(std::vector<channeler::byte> const & buf,
    channeler::message_type type)
{
  assert_message(buf, type, 1, 0);
}



inline void
assert_serialization_ok(std::vector<channeler::byte> & out,
    std::unique_ptr<channeler::message> const & msg, std::vector<channeler::byte> const & buf)
{
  auto res = serialize_message(&out[0], out.size(), msg);
  ASSERT_EQ(res, buf.size());

  for (std::size_t i = 0 ; i < buf.size() ; ++i) {
    // std::cout << "compare index: " << i << std::endl;
    ASSERT_EQ(out[i], buf[i]);
  }
}


} // anonymous namespace

TEST(Message, fail_parse_unknown)
{
  std::vector<channeler::byte> b{message_unknown, message_unknown + message_unknown_size};

  // Exception
  ASSERT_THROW((channeler::message{b.data(), b.size()}),
      channeler::exception);

  // Error code
  channeler::message msg{b.data(), b.size(), false};
  auto err = msg.parse();
  ASSERT_EQ(err.first, channeler::ERR_INVALID_MESSAGE_TYPE);
}



TEST(Message, parse_and_serialize_channel_new)
{
  std::vector<channeler::byte> b{message_channel_new, message_channel_new + message_channel_new_size};

  assert_single_byte_type_fixed_size_message(b, channeler::MSG_CHANNEL_NEW);

  auto msg = channeler::parse_message(b.data(), b.size());
  ASSERT_TRUE(msg);
  ASSERT_EQ(msg->type, channeler::MSG_CHANNEL_NEW);

  auto ptr = reinterpret_cast<channeler::message_channel_new *>(msg.get());
  ASSERT_EQ(0xbeef, ptr->initiator_part);
  ASSERT_EQ(0xbeefb4be, ptr->cookie1);

  // Serialize
  std::vector<channeler::byte> out;
  out.resize(200);
  assert_serialization_ok(out, msg, b);
}



TEST(Message, parse_and_serialize_channel_acknowledge)
{
  std::vector<channeler::byte> b{message_channel_acknowledge, message_channel_acknowledge + message_channel_acknowledge_size};

  assert_single_byte_type_fixed_size_message(b, channeler::MSG_CHANNEL_ACKNOWLEDGE);

  auto msg = channeler::parse_message(b.data(), b.size());
  ASSERT_TRUE(msg);
  ASSERT_EQ(msg->type, channeler::MSG_CHANNEL_ACKNOWLEDGE);

  auto ptr = reinterpret_cast<channeler::message_channel_acknowledge *>(msg.get());
  ASSERT_EQ(0xbeefd00d, ptr->id.full);
  ASSERT_EQ(0xbeefb4be, ptr->cookie1);
  ASSERT_EQ(0xdeadd00d, ptr->cookie2);

  // Serialize
  std::vector<channeler::byte> out;
  out.resize(200);
  assert_serialization_ok(out, msg, b);
}



TEST(Message, parse_and_serialize_channel_finalize)
{
  std::vector<channeler::byte> b{message_channel_finalize, message_channel_finalize + message_channel_finalize_size};

  assert_single_byte_type_fixed_size_message(b, channeler::MSG_CHANNEL_FINALIZE);

  auto msg = channeler::parse_message(b.data(), b.size());
  ASSERT_TRUE(msg);
  ASSERT_EQ(msg->type, channeler::MSG_CHANNEL_FINALIZE);

  auto ptr = reinterpret_cast<channeler::message_channel_finalize *>(msg.get());
  ASSERT_EQ(0xbeefd00d, ptr->id.full);
  ASSERT_EQ(0x3987886e, ptr->cookie2);
  ASSERT_TRUE(ptr->capabilities.none());

  // Serialize
  std::vector<channeler::byte> out;
  out.resize(200);
  assert_serialization_ok(out, msg, b);
}



TEST(Message, parse_and_serialize_channel_cookie)
{
  std::vector<channeler::byte> b{message_channel_cookie, message_channel_cookie + message_channel_cookie_size};

  assert_single_byte_type_fixed_size_message(b, channeler::MSG_CHANNEL_COOKIE);

  auto msg = channeler::parse_message(b.data(), b.size());
  ASSERT_TRUE(msg);
  ASSERT_EQ(msg->type, channeler::MSG_CHANNEL_COOKIE);

  auto ptr = reinterpret_cast<channeler::message_channel_cookie *>(msg.get());
  ASSERT_EQ(0xbeefb4be, ptr->either_cookie);
  ASSERT_TRUE(ptr->capabilities.none());

  // Serialize
  std::vector<channeler::byte> out;
  out.resize(200);
  assert_serialization_ok(out, msg, b);
}



TEST(Message, parse_and_serialize_data)
{
  std::vector<channeler::byte> b{message_data, message_data + message_data_size};

  assert_single_byte_type_variable_size_message(b, channeler::MSG_DATA,
      1);

  auto msg = channeler::parse_message(b.data(), b.size());
  ASSERT_TRUE(msg);
  ASSERT_EQ(msg->type, channeler::MSG_DATA);

  // Serialize
  std::vector<channeler::byte> out;
  out.resize(200);
  assert_serialization_ok(out, msg, b);
}


TEST(Message, iterator_single_message)
{
  std::vector<channeler::byte> b{message_data, message_data + message_data_size};

  channeler::messages msgs{b.data(), b.size()};

  std::size_t count = 0;
  auto iter = msgs.begin();
  for (; iter != msgs.end() ; ++iter, ++count) {
    // std::cout << "asdf type: " << (*iter)->type << std::endl;
    // std::cout << "  remaining: " << iter.remaining() << std::endl;
  }
  ASSERT_EQ(count, 1);
  ASSERT_EQ(iter.remaining(), 0);

  count = 0;
  for (auto x : msgs) {
    ++count;
    // std::cout << "xtype: " << x->type << std::endl;
  }
  ASSERT_EQ(count, 1);
}



TEST(Message, iterator_message_block)
{
  std::vector<channeler::byte> b{message_block, message_block + message_block_size};

  channeler::messages msgs{b.data(), b.size()};

  std::size_t count = 0;
  auto iter = msgs.begin();
  std::vector<channeler::message_type> types;
  for (; iter != msgs.end() ; ++iter, ++count) {
    types.push_back((*iter)->type);
    // std::cout << "asdf type: " << (*iter)->type << std::endl;
    // std::cout << "  remaining: " << iter.remaining() << std::endl;
  }
  ASSERT_EQ(count, 3);
  ASSERT_EQ(iter.remaining(), 4);
  ASSERT_EQ(channeler::MSG_DATA, types[0]);
  ASSERT_EQ(channeler::MSG_CHANNEL_NEW, types[1]);
  ASSERT_EQ(channeler::MSG_CHANNEL_COOKIE, types[2]);

  count = 0;
  types.clear();
  for (auto x : msgs) {
    ++count;
    types.push_back(x->type);
    // std::cout << "xtype: " << x->type << std::endl;
  }
  ASSERT_EQ(count, 3);
  ASSERT_EQ(iter.remaining(), 4);
  ASSERT_EQ(channeler::MSG_DATA, types[0]);
  ASSERT_EQ(channeler::MSG_CHANNEL_NEW, types[1]);
  ASSERT_EQ(channeler::MSG_CHANNEL_COOKIE, types[2]);
}
