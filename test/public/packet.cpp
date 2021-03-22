/**
 * This file is part of channeler.
 *
 * Author(s): Jens Finkhaeuser <jens@finkhaeuser.de>
 *
 * Copyright (c) 2021 Jens Finkhaeuser.
 *
 * This software is licensed under the terms of the GNU GPLv3 for personal,
 * educational and non-profit use. For all other uses, alternative license
 * options are available. Please contact the copyright temp_buffer for additional
 * information, stating your intended usage.
 *
 * You can find the full text of the GPLv3 in the COPYING file in this code
 * distribution.
 *
 * This software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 **/

#include <channeler/packet.h>

#include <gtest/gtest.h>

#include "../packets.h"

using namespace test;

TEST(PacketWrapper, construct_from_buffer_failure_too_small)
{
  std::byte buf[] = { 0xab_b, 0xcd_b };

  ASSERT_THROW((channeler::packet_wrapper{buf, 0}), channeler::exception);
  ASSERT_THROW((channeler::packet_wrapper{buf, sizeof(buf)}), channeler::exception);
}



TEST(PacketWrapper, construct_from_buffer)
{
  temp_buffer data{packet_default_channel_trailing_bytes, packet_default_channel_trailing_bytes_size};

  channeler::packet_wrapper pkt{data.buf.get(), data.size};

  ASSERT_EQ(pkt.proto(), 0xdeadd00d);
  ASSERT_FALSE(pkt.has_valid_proto());

  ASSERT_EQ(pkt.sender().display(), "0x000000000000000000000000000a11c3");
  ASSERT_EQ(pkt.recipient().display(), "0x00000000000000000000000000000b0b");

  // This packet isempty
  ASSERT_EQ(pkt.payload_size(), 0);
  ASSERT_EQ(pkt.packet_size(), pkt.envelope_size());
  ASSERT_EQ(pkt.buffer_size(), packet_default_channel_trailing_bytes_size);

  // We have some flags set though: specifically, 1010 in the least and most
  // significant nibbles.
  ASSERT_FALSE(pkt.flags()[0]);
  ASSERT_TRUE(pkt.flags()[1]);
  ASSERT_FALSE(pkt.flags()[2]);
  ASSERT_TRUE(pkt.flags()[3]);
  for (size_t i = 4 ; i < 12 ; ++i) {
    ASSERT_FALSE(pkt.flags()[i]);
  }
  ASSERT_FALSE(pkt.flags()[12]);
  ASSERT_TRUE(pkt.flags()[13]);
  ASSERT_FALSE(pkt.flags()[14]);
  ASSERT_TRUE(pkt.flags()[15]);

  // With symbolic names
  ASSERT_FALSE(pkt.flag(channeler::FLAG_ENCRYPTED));
  ASSERT_TRUE(pkt.flag(channeler::FLAG_SPIN_BIT));

  // Also ensure that the checksum is correct
  // std::cout << std::hex << pkt.calculate_checksum() << std::dec << std::endl;
  ASSERT_TRUE(pkt.has_valid_checksum());
}



TEST(PacketWrapper, copy)
{
  temp_buffer data{packet_default_channel_trailing_bytes, packet_default_channel_trailing_bytes_size};
  channeler::packet_wrapper pkt0{data.buf.get(), data.size};

  // Create copy
  auto buf = pkt0.copy();
  channeler::packet_wrapper pkt1{buf.get(), pkt0.packet_size()};

  // Both packets must effectively be identical.
  ASSERT_EQ(pkt0.packet_size(), pkt1.packet_size());
  ASSERT_EQ(pkt0.payload_size(), pkt1.payload_size());
  ASSERT_EQ(pkt0.checksum(), pkt1.checksum());
  ASSERT_EQ(pkt0.flags(), pkt1.flags());
  ASSERT_EQ(pkt0.sender(), pkt1.sender());
  ASSERT_EQ(pkt0.recipient(), pkt1.recipient());

  ASSERT_EQ(pkt0.hash(), pkt1.hash());
  ASSERT_EQ(pkt0, pkt1);

  ASSERT_FALSE(pkt0 > pkt1);
  ASSERT_FALSE(pkt0 < pkt1);

  // However - pkt0 will have a larger *buffer* size than pkt1, because
  // the original buffer contained trailing data.
  ASSERT_GT(pkt0.buffer_size(), pkt1.buffer_size());
}



TEST(PacketWrapper, message_iteration)
{
  temp_buffer data{packet_with_messages, packet_with_messages_size};

  channeler::packet_wrapper pkt{data.buf.get(), data.size};

  // We have a payload of 26 Bytes
  ASSERT_EQ(pkt.payload_size(), 26);

  // Iterate over messages and count up message sizes
  std::size_t sum = 0;
  for (auto msg : pkt.get_messages()) {
    sum += msg->buffer_size;
  }
  ASSERT_LT(sum, pkt.payload_size());
  auto diff = pkt.payload_size() - sum;
  ASSERT_EQ(4, diff); // The payload is 4 Bytes larger than the messages
}
