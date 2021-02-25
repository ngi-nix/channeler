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

namespace test {

struct temp_buffer
{
  std::byte * buf;
  size_t size;

  inline temp_buffer(std::byte const * orig, size_t s)
  {
    size = s;
    buf = new std::byte[size];
    memcpy(buf, orig, size);
  }

  inline ~temp_buffer()
  {
    delete [] buf;
  }
};

} // namespace test

#endif // guard
