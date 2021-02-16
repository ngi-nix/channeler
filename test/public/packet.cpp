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

#include <channeler/packet.h>

#include <gtest/gtest.h>

namespace {

inline constexpr std::byte operator "" _b(unsigned long long arg) noexcept
{
  return static_cast<std::byte>(arg);
}


std::byte const packet01[] = {
  // **** public header
  // Proto
  0xde_b, 0xad_b, 0xd0_b, 0x0d_b,

  // Sender
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x0a_b, 0x11_b, 0xc3_b,

  // Recipient
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x00_b, 0x00_b,
  0x00_b, 0x00_b, 0x00_b, 0x00_b,  0x00_b, 0x00_b, 0x0b_b, 0x0b_b,

  // Channel identifier - all zeroes is the default channel
  0x00_b, 0x00_b, 0x00_b, 0x00_b,

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
  0xfa_b, 0xfa_b, 0x25_b, 0xc3_b,

  // **** trailing dat
  // Spurious data in the buffer - this will be ignored
  0xde_b, 0xad_b, 0xbe_b, 0xef_b,
};


struct holder
{
  std::byte * buf;
  size_t size;

  holder(std::byte const * orig, size_t s)
  {
    size = s;
    buf = new std::byte[size];
    memcpy(buf, orig, size);
  }

  ~holder()
  {
    delete [] buf;
  }
};

} // anonymous namespace


TEST(PacketWrapper, construct_from_buffer_failure_too_small)
{
  std::byte buf[] = { 0xab_b, 0xcd_b };

  ASSERT_THROW((channeler::packet_wrapper{buf, 0}), channeler::exception);
  ASSERT_THROW((channeler::packet_wrapper{buf, sizeof(buf)}), channeler::exception);
}



TEST(PacketWrapper, construct_from_buffer)
{
  holder data{packet01, sizeof(packet01)};

  channeler::packet_wrapper pkt{data.buf, data.size};

  ASSERT_EQ(pkt.proto(), 0xdeadd00d);
  ASSERT_FALSE(pkt.has_valid_proto());

  ASSERT_EQ(pkt.sender().display(), "0x000000000000000000000000000a11c3");
  ASSERT_EQ(pkt.recipient().display(), "0x00000000000000000000000000000b0b");

  // This packet isempty
  ASSERT_EQ(pkt.payload_size(), 0);
  ASSERT_EQ(pkt.packet_size(), pkt.envelope_size());
  ASSERT_EQ(pkt.buffer_size(), sizeof(packet01));

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
  holder data{packet01, sizeof(packet01)};
  channeler::packet_wrapper pkt0{data.buf, data.size};

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
  // the original buffer contained spurious data.
  ASSERT_GT(pkt0.buffer_size(), pkt1.buffer_size());
}



TEST(PacketWrapper, copy_packet)
{
  holder data{packet01, sizeof(packet01)};
  channeler::packet_wrapper pkt0{data.buf, data.size};

  // Create copy
  auto pkt1 = pkt0.copy_packet();

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
  // the original buffer contained spurious data.
  ASSERT_GT(pkt0.buffer_size(), pkt1.buffer_size());
}

// TODO
// - message iterations (later)
// - checksum & packet modification -> failures for checksum
