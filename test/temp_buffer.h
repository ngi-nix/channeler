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
#ifndef TEST_TEMP_BUFFER_H
#define TEST_TEMP_BUFFER_H

#include <cstddef>
#include <cstring>
#include <memory>

namespace test {

struct temp_buffer
{
  std::shared_ptr<std::byte>  buf = {};
  size_t                      size = 0;

  inline temp_buffer(std::byte const * orig, size_t s)
  {
    if (!s) {
      return;
    }

    std::byte * b = new std::byte[s];
    memcpy(b, orig, s);

    buf = std::shared_ptr<std::byte>(b);
    size = s;
  }

  inline temp_buffer()
  {
  }

  inline temp_buffer(size_t s)
  {
    if (!s) {
      return;
    }

    std::byte * b = new std::byte[s];
    memset(b, 0, s);

    buf = std::shared_ptr<std::byte>(b);
    size = s;
  }
};

} // namespace test

#endif // guard
