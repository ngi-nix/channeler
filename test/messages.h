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
#ifndef TEST_MESSAGES_H
#define TEST_MESSAGES_H

#include "temp_buffer.h"
#include "byte_suffix.h"

#include <channeler/packet.h>

namespace test {

/**
 * Message definitions
 */
extern std::byte const message_unknown[];
extern std::size_t const message_unknown_size;

extern std::byte const message_channel_new[];
extern std::size_t const message_channel_new_size;

extern std::byte const message_channel_acknowledge[];
extern std::size_t const message_channel_acknowledge_size;

extern std::byte const message_channel_finalize[];
extern std::size_t const message_channel_finalize_size;

extern std::byte const message_channel_cookie[];
extern std::size_t const message_channel_cookie_size;

extern std::byte const message_data[];
extern std::size_t const message_data_size;

// A block of several messages
extern std::byte const message_block[];
extern std::size_t const message_block_size;

} // namespace test

#endif // guard
