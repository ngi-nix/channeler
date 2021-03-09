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
 *
 * As the channelid is defined as having two parts, one chosen by initiator,
 * the other by responder, each party is tasked with selecting a unique part
 * for their role. It is technically sufficient for uniquely identifying a
 * channel.
 *
 * Potential for conflict arises when both peers open a channel simultaneously
 * and choose the same initiator part. In that case, both will log a partial
 * channelid, and send a MSG_CHANNEL_NEW. When they each receive the other
 * peer's message, they will find a pending channel. The correct response is
 * to not respond - this is a fairly rare case, and should be handled by both
 * sides trying again for new channels.
 *
 * But what it means for this class is that we're actually looking up channels
 * only by the initiator part.
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
    auto iter = m_established.find(id.initiator);
    if (iter == m_established.end()) {
      // Initiator not found
      return false;
    }

    if (iter->second.id.responder != id.responder) {
      // Initiator is found, but we have a different channelid remembered.
      // XXX This could actually be a problem.
      return false;
    }
    return true;
  }

  inline bool has_pending_channel(channelid const & id) const
  {
    // Just consider the initiator part here.
    return has_pending_channel(id.initiator);
  }

  inline bool has_pending_channel(channelid::half_type const & initiator) const
  {
    // Just consider the initiator part here.
    return m_pending.end() != m_pending.find(initiator);
  }

  inline void drop_pending_channel(channelid::half_type const & initiator)
  {
    m_pending.erase(initiator);
  }

  inline channelid get_established_id(channelid::half_type const & initiator) const
  {
    auto iter = m_established.find(initiator);
    if (iter != m_established.end()) {
      return iter->second.id;
    }
    return DEFAULT_CHANNELID;
  }


  /**
   * Add - if it's a partial channel identifier, it will be added to
   * the pending set. If it's a full channel identifier, it will create
   * a channel. Otherwise, it's rejected.
   */
  inline error_t add(channelid const & id)
  {
    if (id == DEFAULT_CHANNELID || id.is_complete()) {
      auto iter = m_established.find(id.initiator);
      if (iter != m_established.end()) {
        if (iter->second.id == id) {
          // We can ignore this; the channel is already added. No need to error
          // out, either?
          return ERR_SUCCESS;
        }
        // However, if we currently think that the full channelid is different,
        // we should produce an error.
        return ERR_INVALID_CHANNELID;
      }

      // TODO lock policy pointer
      m_established[id.initiator] = {
        id,
        std::make_shared<channelT>(id, m_packet_size),
      };
      return ERR_SUCCESS;
    }

    if (id.has_initiator()) {
      // This is pending, so we can just overwrite existing identifiers. It
      // makes no difference.
      m_pending.insert(id.initiator);
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
    auto iter = m_established.find(id.initiator);
    if (iter != m_established.end()) {
      if (iter->second.id == id) {
        return ERR_SUCCESS;
      }
      return ERR_INVALID_CHANNELID;
    }

    // If this partial identifier is in the pending set, remove it from there
    // and make a full entry.
    auto iter2 = m_pending.find(id.initiator);
    if (iter2 != m_pending.end()) {
      m_pending.erase(iter2);
    }

    // TODO lock policy pointer
    m_established[id.initiator] = {
      id,
      std::make_shared<channelT>(id, m_packet_size),
    };

    return ERR_SUCCESS;
  }


  /**
   * Retrieve a channel or a nullptr-equivalent for the channel identifier,
   * depending on whether this channel has been established already.
   */
  inline channel_ptr get(channelid const & id) const
  {
    auto iter = m_established.find(id.initiator);
    if (iter != m_established.end()) {
      if (iter->second.id == id) {
        return iter->second.data;
      }
    }
    return {};
  }

private:
  // We need to keep a set of channel identifiers under establishment.
  // TODO: Should we address timeouts of pending channels here? Probably
  //       better in the channel state machine, but that would require
  //       a channel instance.
  using id_set_t = std::unordered_set<channelid::half_type>;
  id_set_t      m_pending;

  // For established channels, we also need to keep a set of channel
  // instances.
  struct value_type
  {
    channelid   id;
    channel_ptr data;
  };

  using channel_map_t = std::unordered_map<channelid::half_type, value_type>;
  channel_map_t m_established;

  // Packet size
  std::size_t m_packet_size;
};

} // namespace channeler

#endif // guard
