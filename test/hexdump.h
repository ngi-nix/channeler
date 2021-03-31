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

#ifndef TEST_HEXDUMP_H
#define TEST_HEXDUMP_H

#include <iostream>
#include <cctype>

namespace test {

/**
 * Dump a buffer of a given size to the output stream, in hexadecimal and
 * ASCII (for printable characters).
 */
void
hexdump(std::ostream & os, char const * buf, std::size_t size);

inline void
hexdump(std::ostream & os, std::byte const * buf, std::size_t size)
{
  return hexdump(os, reinterpret_cast<char const *>(buf), size);
}


} // namespace test

#endif // guard
