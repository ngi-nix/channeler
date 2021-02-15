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

#include <cstring>
#include <iostream> // FIXME

#include <channeler/packet.h>

#include <liberate/serialization/integer.h>

namespace channeler {

namespace {

inline void
update_from_buffer(
    public_header_fields & pub_header,
    std::byte const * buffer,
    size_t buffer_size)
{
  // Read proto ID from buffer
  auto res = liberate::serialization::deserialize_int(pub_header.proto,
      buffer + public_header_layout::PUB_OFFS_PROTO,
      sizeof(pub_header.proto));
  if (res != sizeof(pub_header.proto)) {
    throw std::invalid_argument{"Could not deserialize protocol identifier."};
  }

  // Channel ID
  res = liberate::serialization::deserialize_int(pub_header.channel,
      buffer + public_header_layout::PUB_OFFS_CHANNEL_ID,
      sizeof(pub_header.channel));
  if (res != sizeof(pub_header.channel)) {
    throw std::invalid_argument{"Could not deserialize channel identifier."};
  }

  // Flags
  flags_bits_t bits = 0;
  res = liberate::serialization::deserialize_int(bits,
      buffer + public_header_layout::PUB_OFFS_FLAGS,
      sizeof(bits));
  if (res != sizeof(bits)) {
    throw std::invalid_argument{"Could not deserialize flags."};
  }
  pub_header.flags = flags_t{bits};

  // Packet size
  res = liberate::serialization::deserialize_int(pub_header.packet_size,
      buffer + public_header_layout::PUB_OFFS_PACKET_SIZE,
      sizeof(pub_header.packet_size));
  if (res != sizeof(pub_header.packet_size)) {
    throw std::invalid_argument{"Could not deserialize packet size."};
  }

  if (pub_header.packet_size > buffer_size) {
    throw std::invalid_argument{"Packet size exceeds buffer size."};
  }
}


inline void
update_from_buffer(
    private_header_fields & priv_header,
    std::byte const * buffer,
    size_t packet_size)
{
  // Payload size
  auto res = liberate::serialization::deserialize_int(priv_header.payload_size,
      buffer + private_header_layout::PRIV_OFFS_PAYLOAD_SIZE,
      sizeof(priv_header.payload_size));
  if (res != sizeof(priv_header.payload_size)) {
    throw std::invalid_argument{"Could not deserialize payload size."};
  }

  if (priv_header.payload_size > (packet_size - packet_wrapper::envelope_size())) {
    throw std::invalid_argument{"Payload size exceeds available buffer size."};
  }
}



inline void
update_from_buffer(
    footer_fields & footer,
    std::byte const * buffer,
    size_t buffer_size)
{
  // Read checksum from buffer.
  uint32_t tmp = 0;
  auto res = liberate::serialization::deserialize_int(
      tmp,
      buffer + buffer_size - footer.FOOT_SIZE,
      sizeof(tmp));
  if (res != sizeof(tmp)) {
    throw std::invalid_argument{"Could not deserialize checksum."};
  }
  footer.checksum = tmp;
}



inline void
update_from_buffer(
    public_header_fields & pub_header,
    private_header_fields & priv_header,
    footer_fields & footer,
    std::byte const * buffer,
    size_t buffer_size)
{
  update_from_buffer(pub_header, buffer, buffer_size);
  update_from_buffer(priv_header, buffer + pub_header.PUB_SIZE,
      pub_header.packet_size);

  // Since we've decoded the packet size, we need to calculate the buffer
  // offsets from that size.
  update_from_buffer(footer, buffer, pub_header.packet_size);
}




inline void
update_to_buffer(
    std::byte * buffer,
    size_t buffer_size [[maybe_unused]],
    public_header_fields const & pub_header)
{
  // Write proto ID to buffer
  auto res = liberate::serialization::serialize_int(
      buffer + public_header_layout::PUB_OFFS_PROTO,
      sizeof(pub_header.proto),
      pub_header.proto);
  if (res != sizeof(pub_header.proto)) {
    throw std::invalid_argument{"Could not serialize protocol identifier."};
  }

  // Channel ID
  res = liberate::serialization::serialize_int(
      buffer + public_header_layout::PUB_OFFS_CHANNEL_ID,
      sizeof(pub_header.channel),
      pub_header.channel);
  if (res != sizeof(pub_header.channel)) {
    throw std::invalid_argument{"Could not serialize channel identifier."};
  }

  // Flags
  flags_bits_t bits = pub_header.flags.to_ulong();
  res = liberate::serialization::serialize_int(
      buffer + public_header_layout::PUB_OFFS_FLAGS,
      sizeof(bits),
      bits);
  if (res != sizeof(bits)) {
    throw std::invalid_argument{"Could not serialize flags."};
  }

  // Packet size
  res = liberate::serialization::serialize_int(
      buffer + public_header_layout::PUB_OFFS_PACKET_SIZE,
      sizeof(pub_header.packet_size),
      pub_header.packet_size);
  if (res != sizeof(pub_header.packet_size)) {
    throw std::invalid_argument{"Could not serialize packet size."};
  }
}




inline void
update_to_buffer(
    std::byte * buffer,
    size_t buffer_size [[maybe_unused]],
    private_header_fields const & priv_header)
{
  // Payload size
  auto res = liberate::serialization::serialize_int(
      buffer + private_header_layout::PRIV_OFFS_PAYLOAD_SIZE,
      sizeof(priv_header.payload_size),
      priv_header.payload_size);
  if (res != sizeof(priv_header.payload_size)) {
    throw std::invalid_argument{"Could not serialize payload size."};
  }
}




inline void
update_to_buffer(
    std::byte * buffer,
    size_t buffer_size,
    footer_fields const & footer)
{
  // Write checksum to buffer.
  uint32_t tmp = footer.checksum;
  auto res = liberate::serialization::deserialize_int(
      tmp,
      buffer + buffer_size - footer.FOOT_SIZE,
      sizeof(tmp));
  if (res != sizeof(tmp)) {
    throw std::invalid_argument{"Could not serialize checksum."};
  }
}



inline void
update_to_buffer(
    std::byte * buffer,
    size_t buffer_size,
    public_header_fields const & pub_header,
    private_header_fields const & priv_header,
    footer_fields const & footer)
{
  // We don't really need length checks; this function is entirely internal
  // and will not be called unless a packet has been decoded from the buffer
  // already.
  update_to_buffer(buffer, buffer_size, pub_header);
  update_to_buffer(buffer + pub_header.PUB_SIZE,
      buffer_size - pub_header.PUB_SIZE, priv_header);
  update_to_buffer(buffer, buffer_size, footer);
}


} // anonymous namespace

packet_wrapper::packet_wrapper(std::byte * buf, size_t buffer_size)
  : m_buffer{buf}
  , m_size{buffer_size}
  , m_public_header{m_buffer}
  , m_private_header{}
  , m_footer{}
{
  if (m_size < public_envelope_size()) {
    throw std::out_of_range{"Buffer passed to packet_wrapper is too small to accomodate envelope!"};
  }

  // Update (some) fields from buffer
  update_from_buffer(m_public_header, m_private_header, m_footer, m_buffer,
      m_size);
}



std::byte const *
packet_wrapper::payload() const
{
  return m_buffer + public_header_size() + private_header_size();
}



std::byte const *
packet_wrapper::buffer() const
{
  update_to_buffer(
      const_cast<std::byte *>(m_buffer),
      m_size,
      m_public_header,
      m_private_header,
      m_footer);
  return m_buffer;
}



std::unique_ptr<std::byte[]>
packet_wrapper::copy() const
{
  std::byte * ptr = new std::byte[m_public_header.packet_size];
  std::memcpy(ptr, buffer(), m_public_header.packet_size);
  return std::unique_ptr<std::byte[]>{ptr};
}



liberate::checksum::crc32_checksum
packet_wrapper::calculate_checksum() const
{
  using namespace liberate::checksum;
  std::byte * const end = m_buffer + m_public_header.packet_size
    - footer_size();
  return crc32<CRC32C>(m_buffer, end);
}



bool
packet_wrapper::has_valid_checksum() const
{
  return m_footer.checksum == calculate_checksum();
}



size_t
packet_wrapper::hash() const
{
  // There is no need for a hash calculation when the CRC32 is good enough
  // for this.
  return checksum();
}



bool
packet_wrapper::is_equal_to(packet_wrapper const & other) const
{
  // While CRC32 hash collisions are low enough for the hash() function
  // above, they're not perfect. Let's check common header fields at least.
  return (other.sender() == sender()
      && other.recipient() == recipient()
      && other.channel() == channel()
      && other.flags().to_ulong() == flags().to_ulong()
      && other.packet_size() == packet_size()
      && other.checksum() == checksum());
}



bool
packet_wrapper::is_less_than(packet_wrapper const & other) const
{
  // Similar rationale to is_equal_to()
  if (sender() < other.sender()) {
    return true;
  }

  if (recipient() < other.recipient()) {
    return true;
  }

  if (channel() < other.channel()) {
    return true;
  }

  if (flags().to_ulong() < other.flags().to_ulong()) {
    return true;
  }

  if (packet_size() < other.packet_size()) {
    return true;
  }

  if (checksum() < other.checksum()) {
    return true;
  }

  return false;
}



/*****************************************************************************
 * packet
 */
packet::packet(size_t buffer_size)
  : packet_wrapper{new std::byte[buffer_size], buffer_size}
  , m_ptr{m_buffer}
{
}



} // namespace channeler
