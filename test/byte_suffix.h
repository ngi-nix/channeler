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
#ifndef TEST_BYTE_SUFFIX_H
#define TEST_BYTE_SUFFIX_H

#include <cstddef>
#include <vector>

namespace test {

inline constexpr std::byte operator "" _b(unsigned long long arg) noexcept
{
  return static_cast<std::byte>(arg);
}


inline std::vector<std::byte> operator""_b(char const * str, std::size_t len [[maybe_unused]]) noexcept
{
  auto start = reinterpret_cast<std::byte const *>(str);
  return {start, start + len};
}

} // namespace test

#endif // guard
