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
#ifndef TEST_PACKETS_H
#define TEST_PACKETS_H

#include "temp_buffer.h"
#include "byte_suffix.h"

#include <channeler/packet.h>

namespace test {


/**
 * Packet definitions
 */
extern std::byte const packet_default_channel_trailing_bytes[];
extern std::size_t const packet_default_channel_trailing_bytes_size;

extern std::byte const packet_default_channel[];
extern std::size_t const packet_default_channel_size;

extern std::byte const packet_partial_channelid_initiator[];
extern std::size_t const packet_partial_channelid_initiator_size;

extern std::byte const packet_partial_channelid_responder[];
extern std::size_t const packet_partial_channelid_responder_size;

extern std::byte const packet_regular_channelid[];
extern std::size_t const packet_regular_channelid_size;

} // namespace test

#endif // guard
