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
#ifndef CHANNELER_LOCK_POLICY_H
#define CHANNELER_LOCK_POLICY_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

namespace channeler {

/**
 * Since we want the packet_pool (below) to serialize access only if necessary,
 * we provide a null_lock_policy that does nothing, and use that by default.
 */
struct null_lock_policy
{
  inline void lock() {}
  inline void unlock() {}
};


/**
 * Simple guard for the lock policy
 */
template <typename lockT>
struct guard
{
  inline guard(lockT * lock)
    : m_lock(lock)
  {
    if (m_lock) {
      m_lock->lock();
    }
  }

  inline ~guard()
  {
    if (m_lock) {
      m_lock->unlock();
    }
  }

  lockT * m_lock;
};

/**
 * Specalization of guard for when lockT is the null_lock_policy, because we
 * can optimize for space and branches here.
 */
template <>
struct guard<null_lock_policy>
{
  inline guard(null_lock_policy *)
  {
  }

  inline ~guard()
  {
  }
};


} // namespace channeler

#endif // guard
