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

#include "packets.h"

#include <gtest/gtest.h>
#include <iostream>

namespace test {

std::byte const packet_default_channel_trailing_bytes[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier - see DEFAULT_CHANNELID
  0xf0_b, 0xf0_b, 0xf0_b, 0xf0_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0x32_b, 0xbf_b, 0xf5_b, 0x02_b,

  // **** trailing dat
  // Spurious data in the buffer - this will be ignored
  0xde_b, 0xad_b, 0xbe_b, 0xef_b,
};
std::size_t const packet_default_channel_trailing_bytes_size = sizeof(packet_default_channel_trailing_bytes);



std::byte const packet_default_channel[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier - see DEFAULT_CHANNELID
  0xf0_b, 0xf0_b, 0xf0_b, 0xf0_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0x32_b, 0xbf_b, 0xf5_b, 0x02_b,
};
std::size_t const packet_default_channel_size = sizeof(packet_default_channel);



std::byte const packet_partial_channelid_initiator[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier
  0xd0_b, 0x0d_b, 0xf0_b, 0xf0_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0x1f_b, 0xfe_b, 0xa7_b, 0x8b_b,
};
std::size_t const packet_partial_channelid_initiator_size = sizeof(packet_partial_channelid_initiator);



std::byte const packet_partial_channelid_responder[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier
  0xf0_b, 0xf0_b, 0xd0_b, 0x0d_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0x56_b, 0x0c_b, 0x41_b, 0x20_b,
};
std::size_t const packet_partial_channelid_responder_size = sizeof(packet_partial_channelid_responder);



std::byte const packet_regular_channelid[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size - packet is empty, this includes the envelope
  0x00_b, 0x34_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x00_b,
 
  // **** payload
  // n/a

  // **** footer
  // Checksum
  0x0e_b, 0x77_b, 0x40_b, 0x00_b,
};
std::size_t const packet_regular_channelid_size = sizeof(packet_regular_channelid);



std::byte const packet_with_messages[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Flags
  0xa0_b, 0x0a_b,

  // Packet size
  0x00_b, 0x4e_b,

  // **** private header
  // Sequence number - a random one is fine
  0x01_b, 0xfa_b,

  // Payload size - no payload
  0x00_b, 0x1a_b,
 
  // **** payload
  0x14_b, // MSG_DATA

  0x06_b, // *Payload* size

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

  // **** footer
  // Checksum
  0x02_b, 0xdd_b, 0x6d_b, 0xe1_b,
};
std::size_t const packet_with_messages_size = sizeof(packet_with_messages);




namespace {

#define NAMEOF(var) #var

template <
  typename bufT
>
inline bool validate_packet(char const * const name,
    bufT const & buf, bool expected_value)
{
  // Use a temp buffer, so the packet_wrapper can modify it (not that we want
  // that now).
  std::vector<std::byte> b{buf, buf + sizeof(buf)};
  ::channeler::packet_wrapper p{b.data(), b.size()};
  if (p.has_valid_checksum() != expected_value) {
    std::cout << "Packet: " << name << std::endl;
    std::cout << "  Checksum:   " << std::hex << p.checksum() << std::dec << std::endl;
    std::cout << "  Calculated: " << std::hex << p.calculate_checksum() << std::dec << std::endl;
    return false;
  }
  return true;
}

} // anonymous namespace
} // namespace test


TEST(TestPacket, validity)
{
  using namespace test;

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_default_channel_trailing_bytes),
          packet_default_channel_trailing_bytes,
          true)));

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_default_channel),
          packet_default_channel,
          true)));

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_partial_channelid_initiator),
          packet_partial_channelid_initiator,
          true)));

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_partial_channelid_responder),
          packet_partial_channelid_responder,
          true)));

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_regular_channelid),
          packet_regular_channelid,
          true)));

  ASSERT_TRUE((validate_packet(
          NAMEOF(packet_with_messages),
          packet_with_messages,
          true)));
}
