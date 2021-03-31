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
#include <sstream>
#include <iomanip>
#include <cctype>

#include "hexdump.h"

namespace test {

template <typename T>
inline std::ostream &
tohex(std::ostream & os, T val, int pad = 8)
{
  os << std::hex << std::setfill('0') << std::setw(pad)
    << (int) val
    << std::setw(0) << std::dec;
  return os;
}

void
hexdump(std::ostream & os, char const * buf, std::size_t size)
{
  constexpr size_t const bytes_per_line = 32;
  constexpr size_t const bytes_per_column = 4;

  char asciibuf[bytes_per_line] = { 0 };
  size_t asciioff = 0;

  size_t i = 0;
  for (; i < size - 1; ++i) {
    if ((i % bytes_per_line) == 0) {
      if (asciioff > 0) {
        os << "   " << std::string{asciibuf, asciibuf + asciioff};
      }
      asciioff = 0;
      os << std::endl;
      tohex(os, i) << ":";
    }
    if ((i % bytes_per_column) == 0) {
      os << " ";
    }

    char c = *(buf + i);
    tohex<uint8_t>(os, c, 2);
    asciibuf[asciioff++] = std::isprint(c) && c != ' ' ? c : '.';
  }

  // Left over ASCII buffer
  size_t to_pad = bytes_per_line - asciioff;
  for (; i < size + to_pad - 1; ++i) {
    if ((i % bytes_per_column) == 0) {
      os << " ";
    }
    os << "..";
  }
  os << "   " << std::string{asciibuf, asciibuf + asciioff};
  os << std::endl;
}

} // namespace test
