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
#ifndef CHANNELER_CHANNELS_H
#define CHANNELER_CHANNELS_H

#ifndef __cplusplus
#error You are trying to include a C++ only header file
#endif

#include <channeler.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <channeler/channelid.h>

namespace channeler {

/**
 * The channels class is a holder for channels.
 *
 * In particular, it takes care that within each instance (aka connection),
 * a) Channel identifiers are not duplicated, and
 * b) The (abstract) state of channels is tracked.
 *
 * Channel state can take the form of one of three states:
 * a) unknown/absent,
 * b) pending (under establishment), and
 * c) established.
 *
 * For the latter state, a channel instance will be created and maintained
 * as long as the channel is alive.
 */
template <
  typename channelT
  // TODO Lock policy? null policy should be in its own header
>
class channels
{
public:
  using channel_ptr = std::shared_ptr<channelT>;

  inline channels(std::size_t packet_size)
    : m_packet_size(packet_size)
  {
  }

  inline bool has_channel(channelid const & id) const
  {
    return has_established_channel(id) || has_pending_channel(id);
  }

  inline bool has_established_channel(channelid const & id) const
  {
    return m_established.end() != m_established.find(id);
  }

  inline bool has_pending_channel(channelid const & id) const
  {
    auto partial = id.create_partial();
    return m_pending.end() != m_pending.find(partial);
  }


  /**
   * Add - if it's a partial channel identifier, it will be added to
   * the pending set. If it's a full channel identifier, it will create
   * a channel. Otherwise, it's rejected.
   */
  inline error_t add(channelid const & id)
  {
    if (id == DEFAULT_CHANNELID || id.is_complete()) {
      auto iter = m_established.find(id);
      if (iter != m_established.end()) {
        // We can ignore this; the channel is already added. No need to error
        // out, either?
        return ERR_SUCCESS;
      }

      // TODO lock policy pointer
      m_established[id] = std::make_shared<channelT>(id, m_packet_size);
      return ERR_SUCCESS;
    }

    if (id.has_initiator()) {
      // This is pending, so we can just overwrite existing identifiers. It
      // makes no difference.
      m_pending.insert(id);
      return ERR_SUCCESS;
    }

    // An uninitalized identifier or one with just the responder part is
    // invalid.
    return ERR_INVALID_CHANNELID;
  }


  /**
   * Update a channel identifier. This is tricky, because it's mostly about
   * moving a partial channel from the pending set to a fully established
   * state.
   */
  inline error_t make_full(channelid const & id)
  {
    // First, the passed id must be a full id.
    if (!id.is_complete()) {
      return ERR_INVALID_CHANNELID;
    }

    // If the id is already in the set, we can just ignore the call.
    auto iter = m_established.find(id);
    if (iter != m_established.end()) {
      return ERR_SUCCESS;
    }

    // Next, we need to construct a new channel identifier from the passed one
    // that is partial, i.e. has an initiator.
    auto partial = id.create_partial();

    // If this partial identifier is in the pending set, remove it from there
    // and make a full entry.
    auto iter2 = m_pending.find(partial);
    if (iter2 != m_pending.end()) {
      m_pending.erase(iter2);
    }

    // TODO lock policy pointer
    m_established[id] = std::make_shared<channelT>(id, m_packet_size);

    return ERR_SUCCESS;
  }


  /**
   * Retrieve a channel or a nullptr-equivalent for the channel identifier,
   * depending on whether this channel has been established already.
   */
  inline channel_ptr get(channelid const & id) const
  {
    auto iter = m_established.find(id);
    if (iter != m_established.end()) {
      return iter->second;
    }
    return {};
  }

private:
  // We need to keep a set of channel identifiers under establishment.
  // TODO: Should we address timeouts of pending channels here? Probably
  //       better in the channel state machine, but that would require
  //       a channel instance.
  using id_set_t = std::unordered_set<channelid>;
  id_set_t      m_pending;

  // For established channels, we also need to keep a set of channel
  // instances.
  using channel_map_t = std::unordered_map<channelid, channel_ptr>;
  channel_map_t m_established;

  // Packet size
  std::size_t m_packet_size;
};

} // namespace channeler

#endif // guard
