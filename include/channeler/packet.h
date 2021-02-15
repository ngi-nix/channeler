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
#ifndef CHANNELER_PACKET_H
#define CHANNELER_PACKET_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <bitset>

#include <liberate/cpp/operators/comparison.h>
#include <liberate/cpp/hash.h>
#include <liberate/checksum/crc32.h>

#include <channeler/protoid.h>
#include <channeler/peerid.h>

namespace channeler {

using channel_id = uint32_t; // FIXME

/**
 * Packet/payload size types
 */
using packet_size_t = uint16_t;
using payload_size_t = packet_size_t;

/**
 * Flags representation
 */
using flags_bits_t = uint16_t;
constexpr size_t FLAG_COUNT = sizeof(flags_bits_t) * 8;
using flags_t = std::bitset<FLAG_COUNT>;

/**
 * This file contains classes related to handling networking packets.
 *
 * One of the peculiarities about channeler is that we want to provide
 * as little information to eavesdroppers as possible; as such, we will
 * happily send more *Bytes* than necessary to serialize a packet's
 * messages, i.e. padding.
 *
 * That means we are working with a relatively fixed packet size, but a
 * variable payload size. The relationship is this:
 *
 * - packet size: the size in Bytes of the entire packet
 * - envelope size: header size + footer size
 *   - header size: the number of Bytes of header information
 *   - footer size: the number of Bytes of footer information
 * - payload size: the number of Bytes of actual payload data
 *    as opposed to padding.
 * - padding size: packet size - envelope size - payload size
 *
 * Here is the packet layout, broadly speaking:
 *
 *    +--------+---------+---------+--------+
 *    | Header | Payload | Padding | Footer |
 *    +--------+---------+---------+--------+
 *
 * The header information contains the size of the payload, i.e. the size of
 * the part of the packet buffer between header and footer that contains non-
 * padding data.
 *
 * This arrangement is fine when fixed-sized packets are sent, or when
 * datagrams are received via the recv() function which returns the packet
 * size. Over stream-oriented transports, it's not possible to delinate packet
 * boundaries, at which point the size of the padding can no longer be
 * calculated.
 *
 * We therefore encapsulate both the packet length and the payload length
 * in the header: the packet length is largely used for stream-oriented
 * protocols, and allows reading first a header sized buffer followed by
 * a buffer containing the rest of the packet (ignoring memory allocation for
 * the moment). The payload length follows the header proper.
 *
 * Of course, when packets are encrypted, it is important to also encrypt this
 * payload length and consequently the trailing padding. We more properly
 * differentiate between a public and a private header in the packet layout.
 *
 *        unencrypted          potentially encrypted       unencrypted
 *     _______|______   __________________|_______________   ___|__
 *    /              \ /                                  \ /      \
 *    +---------------+----------------+---------+---------+--------+
 *    | Public Header | Private Header | Payload | Padding | Footer |
 *    +---------------+----------------+---------+---------+--------+
 *
 * Implementations are free to choose any values for the padding, but must
 * take care not to leak uninitialized memory here. A pattern such as in e.g.
 * PKCS#7/RFC5652 is recommended for this reason, but the padding has no
 * semantics. Note also that the padding size should be chosen such that the
 * combined sizes of the private header, payload and padding no longer require
 * additional padding from the encryption algorithm chosen.
 *
 * We encapsulate this concept in the classes below.
 *
 * Note also that we largely assume a fixed packet size implementation - this
 * is useful for keeping padding to below a chosen MTU, ensuring that the
 * cryptographic requirements above are adhered to, etc. But the classes below
 * should accept any size packet and work with it.
 */

/**
 * Public header layout - this is most easily expressed as constants providing
 * for something of a single source of truth.
 */
struct public_header_layout
{
  static constexpr size_t PUB_OFFS_PROTO = 0;
  static constexpr size_t PUB_OFFS_SENDER = sizeof(protoid);
  static constexpr size_t PUB_OFFS_RECIPIENT = PUB_OFFS_SENDER + peerid::size();
  static constexpr size_t PUB_OFFS_CHANNEL_ID = PUB_OFFS_RECIPIENT + peerid::size();
  static constexpr size_t PUB_OFFS_FLAGS = PUB_OFFS_CHANNEL_ID + sizeof(channel_id);
  static constexpr size_t PUB_OFFS_PACKET_SIZE = PUB_OFFS_FLAGS + (FLAG_COUNT / 8);

  static constexpr size_t PUB_SIZE = PUB_OFFS_PACKET_SIZE + sizeof(packet_size_t);
};

/**
 * Private header layout.
 */
struct private_header_layout
{
  static constexpr size_t PRIV_OFFS_PAYLOAD_SIZE = 0;

  static constexpr size_t PRIV_SIZE = PRIV_OFFS_PAYLOAD_SIZE + sizeof(payload_size_t);
};

/**
 * (Public) footer layout
 */
struct footer_layout
{
  // Footer offset is negative.
  static constexpr ssize_t FOOT_OFFS_CHECKSUM = -sizeof(uint32_t); // CRC32

  static constexpr size_t FOOT_SIZE = -1 * FOOT_OFFS_CHECKSUM;
};


/**
 * Classes for public/private header and footer
 */
struct CHANNELER_API public_header_fields
  : public public_header_layout
{
  // Deserialized public header info
  protoid         proto = PROTOID;

  peerid_wrapper  sender;
  peerid_wrapper  recipient;

  channel_id      channel;

  flags_t         flags;

  packet_size_t   packet_size;

  public_header_fields(std::byte * buf)
    : sender{buf + PUB_OFFS_SENDER}
    , recipient{buf + PUB_OFFS_RECIPIENT}
  {
  }
};


struct CHANNELER_API private_header_fields
  : public public_header_layout
{
  payload_size_t  payload_size;
};


struct CHANNELER_API footer_fields
  : public footer_layout
{
  liberate::checksum::crc32_checksum  checksum;
};



/**
 * Packet wrapper - initialized with a byte buffer, offers a representation
 * of the packet header fields and messages.
 *
 * This class does not *own* the data buffer it receives.
 */
class CHANNELER_API packet_wrapper
  : public ::liberate::cpp::comparison_operators<packet_wrapper>
{
protected:
  // Keep these here - this is so that buffer and size are initialized before
  // any other fields. Also keep the entire private section at the top.
  std::byte * m_buffer;
  size_t      m_size;

  // Deserialized info
  public_header_fields  m_public_header;
  private_header_fields m_private_header;
  footer_fields         m_footer;

public:

  /**
   * Construct with a raw byte buffer. The class does not take ownership of the
   * buffer, but merely presents a packet interface for it. This means
   * (de-)serializing from/to the buffer as necessary.
   */
  packet_wrapper(std::byte * buf, size_t buffer_size);

  /**
   * Field accessors
   */
  inline protoid proto() const
  {
    return m_public_header.proto;
  }


  inline peerid_wrapper sender() const
  {
    return m_public_header.sender;
  }

  inline peerid_wrapper sender()
  {
    return m_public_header.sender;
  }


  inline peerid_wrapper recipient() const
  {
    return m_public_header.recipient;
  }

  inline peerid_wrapper recipient()
  {
    return m_public_header.recipient;
  }


  inline channel_id channel() const
  {
    return m_public_header.channel;
  }

  inline channel_id channel()
  {
    return m_public_header.channel;
  }


  inline flags_t flags() const
  {
    return m_public_header.flags;
  }

  inline flags_t flags()
  {
    return m_public_header.flags;
  }


  inline packet_size_t packet_size() const
  {
    return m_public_header.packet_size;
  }

  inline payload_size_t payload_size() const
  {
    return m_private_header.payload_size;
  }

  inline liberate::checksum::crc32_checksum checksum() const
  {
    return m_footer.checksum;
  }


  // TODO
  // - has_valid_protoid()
  // - is_encrypted()
  //  -> enum for bitset positions; make a simpler getter/setter that
  //     just updates bitset
  // - modifiers

  /**
   * Metadata helpers
   */
  static constexpr size_t public_header_size()
  {
    return public_header_layout::PUB_SIZE;
  }

  static constexpr size_t private_header_size()
  {
    return private_header_layout::PRIV_SIZE;
  }

  static constexpr size_t footer_size()
  {
    return footer_layout::FOOT_SIZE;
  }

  static constexpr size_t public_envelope_size()
  {
    return public_header_size() + footer_size();
  }

  static constexpr size_t envelope_size()
  {
    return public_envelope_size() + private_header_size();
  }

  /**
   * Size of the buffer passed to constructor
   */
  size_t buffer_size() const
  {
    return m_size;
  }

  /**
   * Maximum payload size - this is the buffer minus the envelope.
   */
  size_t max_payload_size() const
  {
    return buffer_size() - envelope_size();
  }


  /**
   * Return a pointer to the start of the payload.
   */
  std::byte const * payload() const;

  /**
   * Return a pointer to the start of the buffer.
   */
  std::byte const * buffer() const;

  /**
   * Copy packet_size() Bytes from the buffer intoa new buffer, and return
   * that.
   */
  std::unique_ptr<std::byte[]> copy() const;

  /**
   * Calculate and validate checksum
   */
  liberate::checksum::crc32_checksum calculate_checksum() const;
  bool has_valid_checksum() const;

  /**
   * Value type semantics
   */
  size_t hash() const;

  bool is_equal_to(packet_wrapper const & other) const;
  bool is_less_than(packet_wrapper const & other) const;
};


/**
 * A packet is a packet_wrapper that manages an internal packet buffer.
 */
class CHANNELER_API packet
  : public packet_wrapper
{
public:
  packet(size_t buffer_size);

private:
  std::shared_ptr<std::byte[]> m_ptr;
};

// TODO
// - packet_holder (wrapper that takes ownership)
// - packet (packet_holder that allocates); needs MTU
//   OR payload size OR either

} // namespace channeler


LIBERATE_MAKE_HASHABLE(channeler::packet_wrapper)

#endif // guard
