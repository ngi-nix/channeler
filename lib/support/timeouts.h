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
#ifndef CHANNELER_SUPPORT_TIMEOUTS_H
#define CHANNELER_SUPPORT_TIMEOUTS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <chrono>
#include <functional>
#include <map>
#include <unordered_set>
#include <vector>

#include <liberate/cpp/hash.h>
#include <liberate/cpp/operators.h>


namespace channeler::support {

/**
 * In terms of context, each timeout is registered with a scope and tag value.
 * Each of those is a simple integer value. Scopes should be e.g. something
 * like a CHANNEL_ESTABLISHMENT value, while the tag might be the channel
 * identifier in question, or some similar context.
 */
using timeout_scope_type = uint16_t;
using timeout_tag_type = uint16_t;

union timeout_scoped_tag_type
{
  uint32_t _hash;
  struct {
    timeout_scope_type  scope;
    timeout_tag_type    tag;
  };

  inline timeout_scoped_tag_type(
      timeout_scope_type _scope,
      timeout_tag_type _tag
    )
    : scope{_scope}
    , tag{_tag}
  {
  }

  inline bool operator==(timeout_scoped_tag_type const & other) const
  {
    return _hash == other._hash;
  }

  inline bool operator<(timeout_scoped_tag_type const & other) const
  {
    return _hash < other._hash;
  }

  inline std::size_t hash() const
  {
    return _hash;
  }
};

} // namespace channeler::support

LIBERATE_MAKE_HASHABLE(channeler::support::timeout_scoped_tag_type)
LIBERATE_MAKE_COMPARABLE(channeler::support::timeout_scoped_tag_type)


namespace channeler::support {

/**
 * We want a support class for handling timeouts. This support class takes
 * care of assiging some kind of meaning to a timeout, while it expects some
 * platform-provided functionality (e.g. in tests, a test-platform) with
 * minimal expectations.
 *
 * In the case of timeouts, the best we can really expect from a platform is
 * a function that sleeps for a maximum given time period, and returns the
 * actual duration slept. We'll simplify the API a little by assuming that
 * this is given in duration - each platform implementation
 * can provide its own interpretation of that, then.
 *
 * The sleep function takes the duration to sleep for, and returns the actually
 * elapsed duration.
 *
 */

class timeouts
{
public:
  using duration = std::chrono::nanoseconds;

  using sleep_function = std::function<
    duration (duration)
  >;

  using expired = std::vector<timeout_scoped_tag_type>;

  inline timeouts(sleep_function && func)
    : m_sleep{func}
  {
  }

  /**
   * Add a *transient* timeout with the scoped tag, i.e. when it expires,
   * it also gets removed from the timeouts class.
   */
  inline bool add(timeout_scoped_tag_type scoped_tag, duration amount)
  {
    if (m_tags.find(scoped_tag) != m_tags.end()) {
      return false;
    }

    m_tags.insert(scoped_tag);
    m_timeouts.insert({amount, scoped_tag});
    return true;
  }

  /**
   * Remove a transient timeout without expiring it.
   */
  inline void remove(timeout_scoped_tag_type scoped_tag)
  {
    auto iter = m_tags.find(scoped_tag);
    if (iter == m_tags.end()) {
      return;
    }

    m_tags.erase(iter);

    auto iter2 = m_timeouts.begin();
    for ( ; iter2 != m_timeouts.end() ; ++iter2) {
      if (iter2->second == scoped_tag) {
        // We can only erase in the loop because we exit the loop!
        m_timeouts.erase(iter2);
        break;
      }
    }
  }

  /**
   * Wait for the given duration, and return all expired timeouts.
   */
  inline expired wait(duration amount)
  {
    // This implementation is a bit brute force, but it allows the platform
    // expectations to be relatively low. If it becomes a performance issue,
    // we can always optimize it.
    // TODO:
    // - check if this needs optimization
    // - may need an entirely different approach to integrate well with a
    //   platform's I/O system
    //   https://gitlab.com/interpeer/channeler/-/issues/10

    auto elapsed = m_sleep(amount);
    map_t remaining;
    expired result;
    for (auto entry : m_timeouts) {
      // If the entry's duration is less or equal to elapsed, then
      // return it.
      if (entry.first <= elapsed) {
        result.push_back(entry.second);
      }
      else {
        // Otherwise, the entry has to remain - but we adjust the expectation
        // of the duration downwards.
        remaining.insert({entry.first - elapsed, entry.second});
      }
    }

    m_timeouts = remaining;

    return result;
  }

private:
  sleep_function  m_sleep;

  using set_t = std::unordered_set<timeout_scoped_tag_type>;
  set_t           m_tags = {};

  using map_t = std::multimap<duration, timeout_scoped_tag_type>;
  map_t           m_timeouts = {};
};



} // namespace channeler::support


#endif // guard
