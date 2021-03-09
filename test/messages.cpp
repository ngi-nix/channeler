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

#include "messages.h"

#include <gtest/gtest.h>
#include <iostream>

namespace test {

std::byte const message_unknown[] = {
  0x7f_b, // Nothing, but below one byte

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // junk
};
std::size_t const message_unknown_size = sizeof(message_unknown);



std::byte const message_channel_new[] = {
  0x0a_b, // MSG_CHANNEL_NEW

  0xbe_b, 0xef_b, // Half channel ID

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // crc32 (cookie)
};
std::size_t const message_channel_new_size = sizeof(message_channel_new);



std::byte const message_channel_acknowledge[] = {
  0x0b_b, // MSG_CHANNEL_ACKNOWLEDGE

  0xbe_b, 0xef_b, 0xd0_b, 0x0d_b, // Channel ID

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // crc32 (cookie)
};
std::size_t const message_channel_acknowledge_size = sizeof(message_channel_acknowledge);



std::byte const message_channel_finalize[] = {
  0x0c_b, // MSG_CHANNEL_FINALIZE

  0xbe_b, 0xef_b, 0xd0_b, 0x0d_b, // Channel ID

  0x39_b, 0x87_b, 0x88_b, 0x6e_b, // crc32 (cookie); used in FSM for channel responder

  0x00_b, 0x00_b, // Capabilities
};
std::size_t const message_channel_finalize_size = sizeof(message_channel_finalize);



std::byte const message_channel_cookie[] = {
  0x0d_b, // MSG_CHANNEL_COOKIE

  // Channel ID is in header

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // crc32 (cookie)

  0x00_b, 0x00_b, // Capabilities
};
std::size_t const message_channel_cookie_size = sizeof(message_channel_cookie);




std::byte const message_data[] = {
  0x14_b, // MSG_DATA

  0x08_b, // *Message* size

  // Payload
  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, 0x00_b, 0x00_b,
};
std::size_t const message_data_size = sizeof(message_data);



std::byte const message_block[] = {
  0x14_b, // MSG_DATA

  0x08_b, // *Message* size

  // Payload
  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, 0x00_b, 0x00_b,

  // ---
  0x0a_b, // MSG_CHANNEL_NEW

  0xbe_b, 0xef_b, // Half channel ID

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // crc32 (cookie)

  // ---
  0x0d_b, // MSG_CHANNEL_COOKIE

  // Channel ID is in header

  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // crc32 (cookie)

  0x00_b, 0x00_b, // Capabilities

  // ---
  0xbe_b, 0xef_b, 0xb4_b, 0xbe_b, // junk
};
std::size_t const message_block_size = sizeof(message_block);




} // namespace test
