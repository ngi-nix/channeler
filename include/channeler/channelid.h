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
#ifndef CHANNELER_CHANNELID_H
#define CHANNELER_CHANNELID_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <cstddef>
#include <ostream>
#include <iomanip>

#include <liberate/cpp/operators.h>
#include <liberate/cpp/hash.h>

namespace channeler {

/**
 * Channel identifiers are really just simple numbers. The interesting part is
 * that one half of the bits are set by the side requesting to initiate a new
 * channel, the other half of the bits are set by the other side. There is no
 * strict separation of client or server, this is a per-channel operation.
 *
 * This header contains helper functionality for working with such channel
 * identifiers.
 */
union CHANNELER_API channelid
{
  using full_type = uint32_t;
  using half_type = uint16_t;

  full_type full;
  CHANNELER_ANONYMOUS struct {
    half_type initiator;   // First (most significant) bits are set by the
                           // initiating side.
    half_type responder;   // The responding side fills in the rest.
  };

  inline channelid(half_type init, half_type resp)
    : initiator{init}
    , responder{resp}
  {
  }

  inline constexpr channelid()
    : full{0xF0F0F0F0uL}
  {
  }

  // Behave like a value type
  inline size_t hash() const
  {
    return full;
  }

  inline bool operator==(channelid const & other) const
  {
    return full == other.full;
  }

  inline bool operator<(channelid const & other) const
  {
    return full < other.full;
  }

  // Verification
  inline bool has_initiator() const
  {
    return initiator != half_type{0xF0F0};
  }

  inline bool has_responder() const
  {
    return responder != half_type{0xF0F0};
  }

  inline bool is_complete() const
  {
    return has_initiator() && has_responder();
  }

  inline channelid create_partial() const
  {
    auto ret = *this;
    ret.responder = half_type{0xF0F0};
    return ret;
  }
};

LIBERATE_MAKE_COMPARABLE(channelid)

/**
 * Output operator
 */
inline std::ostream &
operator<<(std::ostream & os, channelid const & id)
{
  os << "["
    << std::hex << std::setw(sizeof(channelid::half_type) * 2) << std::setfill('0') << id.initiator
    << ":" << id.responder << "]"
    << std::dec << std::endl;
  return os;
}


/**
 * The identifier for the default channel does not require negotiation. It's
 * a fixed pattern of bits; we choose to alternate between set and unset
 * nibbles to make it a little more distinguishable.
 */
constexpr channelid DEFAULT_CHANNELID = channelid{};


/**
 * Creating a new channel identifier means randomly filling the initiator
 * bits and not colliding with the bit pattern of the DEFAULT_CHANNELID.
 *
 * The initiator must also create a unique identifier, but that is outside
 * the scope of a type - it requires knowledge of which identifiers are in
 * use between two peers.
 */
CHANNELER_API
channelid create_new_channelid();


/**
 * Similarly, completing a channel identifier means filling in the responder
 * bits, and not colliding with the DEFAULT_CHANNELID bit pattern.
 *
 * Also here, a peer must make sure to produce unique identifiers.
 *
 * The function returns an error if the channel identifier could not be
 * completed.
 */
CHANNELER_API
error_t complete_channelid(channelid & id);

} // namespace channeler


LIBERATE_MAKE_HASHABLE(channeler::channelid)



#endif // guard
