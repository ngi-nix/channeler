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

#include "../lib/internal/api.h"
#include "../lib/context/node.h"
#include "../lib/context/connection.h"

#include "../../hexdump.h"

#include <gtest/gtest.h>

namespace {

static constexpr std::size_t PACKET_SIZE = 120;

using address_t = int; // for testing

using node_t = ::channeler::context::node<
  3 // POOL_BLOCK_SIZE
  // XXX lock policy is null by default
>;

using connection_t = ::channeler::context::connection<
  address_t,
  node_t
>;

using api_t = channeler::internal::connection_api<
  connection_t
>;


constexpr char const hello[] = "hello, world!";
constexpr std::size_t hello_size = sizeof(hello);


struct packet_loop_callback
{
  inline packet_loop_callback(api_t *& self, api_t *& peer)
    : m_self{self}
    , m_peer{peer}
  {
  }

  void packet_to_send(channeler::channelid const & channel)
  {
    ++m_call_count;

    // If we have a packet to send, we'll just feed it to our peer.
    auto entry = m_self->packet_to_send(channel);
    test::hexdump(std::cerr, entry.packet.buffer(), entry.packet.buffer_size());

    // Allocate a peer slot, copy packet.
    auto peer_slot = m_peer->allocate();
    ASSERT_EQ(entry.packet.buffer_size(), peer_slot.size());
    memcpy(peer_slot.data(), entry.packet.buffer(), peer_slot.size());

    test::hexdump(std::cerr, peer_slot.data(), peer_slot.size());

    // Let the peer consume its slot
    m_peer->received_packet(123, 321, peer_slot);
  }

  std::size_t m_call_count = 0;
  api_t *&    m_self;
  api_t *&    m_peer;
};


struct channel_establishment_callback
{
  channeler::channelid m_id = channeler::DEFAULT_CHANNELID;

  void callback(channeler::error_t err, channeler::channelid const & id)
  {
    ASSERT_EQ(channeler::ERR_SUCCESS, err);
    m_id = id;
  }
};


struct data_available_callback
{
  channeler::channelid  m_id = channeler::DEFAULT_CHANNELID;
  std::size_t           m_size = 0;

  void callback(channeler::channelid const & id, std::size_t size)
  {
    m_id = id;
    m_size = size;
  }
};


template <typename peer_apiT>
inline void
test_data_exchange(channeler::channelid const & id,
    std::string const & message,
    peer_apiT & peer1,
    data_available_callback const & callback2,
    peer_apiT & peer2)
{
  using namespace channeler;

  // Write to peer1
  std::size_t written = 0;
  auto err = peer1.channel_write(id, message.c_str(), message.size(), written);
  ASSERT_EQ(ERR_SUCCESS, err);
  ASSERT_EQ(written, message.size());

  // Peer2's data callback must have been called.
  ASSERT_NE(callback2.m_id, DEFAULT_CHANNELID);
  ASSERT_EQ(callback2.m_id, id);
  ASSERT_EQ(callback2.m_size, message.size());

  // Read from peer2
  std::vector<char> buf;
  buf.resize(message.size() * 2);

  std::size_t read = 0;
  err = peer2.channel_read(id, &buf[0], buf.size(), read);
  ASSERT_EQ(ERR_SUCCESS, err);
  ASSERT_EQ(read, message.size());
  for (std::size_t i = 0 ; i < read ; ++i) {
    ASSERT_EQ(message[i], buf[i]);
  }
}

// All test cases
static channeler::peerid self;
static channeler::peerid peer;

static node_t self_node{
  self,
  PACKET_SIZE,
  []() -> std::vector<std::byte> { return {}; },
  [](channeler::support::timeouts::duration d) { return d; },
};

static node_t peer_node{
  peer,
  PACKET_SIZE,
  []() -> std::vector<std::byte> { return {}; },
  [](channeler::support::timeouts::duration d) { return d; },
};



} // anonymous namespace


TEST(InternalAPI, create)
{
  using namespace channeler::fsm;

  connection_t ctx{self_node, peer};

  api_t api{
    ctx,
    [](error_t, channeler::channelid) {},
    [](channeler::channelid){},
    [](channeler::channelid, std::size_t) {}
  };
}


TEST(InternalAPI, fail_sending_data_on_default_channel)
{
  using namespace channeler::fsm;
  using namespace channeler;

  connection_t ctx{self_node, peer};

  api_t api{
    ctx,
    [](channeler::error_t, channelid) {},
    [](channeler::channelid){},
    [](channelid, std::size_t) {}
  };

  std::size_t written = 0;

  auto err = api.channel_write(DEFAULT_CHANNELID,
      hello, hello_size, written);
  ASSERT_EQ(ERR_INVALID_CHANNELID, err);
  ASSERT_EQ(0, written);
}


TEST(InternalAPI, establish_channel)
{
  using namespace channeler::fsm;
  using namespace channeler;

  connection_t ctx1{self_node, peer};
  connection_t ctx2{peer_node, self};

  api_t * peer_api1 = nullptr;
  api_t * peer_api2 = nullptr;

  packet_loop_callback loop1{peer_api1, peer_api2};
  packet_loop_callback loop2{peer_api2, peer_api1};

  channel_establishment_callback ccb1;
  channel_establishment_callback ccb2;

  using namespace std::placeholders;

  peer_api1 = new api_t{
    ctx1,
    [](channeler::error_t, channelid) {},
    std::bind(&packet_loop_callback::packet_to_send, &loop1, _1),
    [](channelid, std::size_t) {}
  };
  peer_api2 = new api_t{
    ctx2,
    std::bind(&channel_establishment_callback::callback, &ccb2, _1, _2),
    std::bind(&packet_loop_callback::packet_to_send, &loop2, _1),
    [](channelid, std::size_t) {}
  };

  // Before channel establishment: defaults
  ASSERT_EQ(DEFAULT_CHANNELID, ccb1.m_id);
  ASSERT_EQ(DEFAULT_CHANNELID, ccb2.m_id);

  // Establish channel
  auto err = peer_api1->establish_channel(
    ctx2.node().id(),
    std::bind(&channel_establishment_callback::callback, &ccb1, _1, _2)
  );
  ASSERT_EQ(ERR_SUCCESS, err);
  std::cout << loop1.m_call_count << std::endl;
  std::cout << loop2.m_call_count << std::endl;


  // After channel establishment: identifiers are non-default and must match
  ASSERT_NE(DEFAULT_CHANNELID, ccb1.m_id);
  ASSERT_NE(DEFAULT_CHANNELID, ccb2.m_id);
  ASSERT_EQ(ccb1.m_id, ccb2.m_id);

  std::cout << loop1.m_call_count << std::endl;
  std::cout << loop2.m_call_count << std::endl;

  delete peer_api1;
  delete peer_api2;
}


TEST(InternalAPI, send_data_on_established_channel)
{
  using namespace channeler::fsm;
  using namespace channeler;

  connection_t ctx1{self_node, peer};
  connection_t ctx2{peer_node, self};

  api_t * peer_api1 = nullptr;
  api_t * peer_api2 = nullptr;

  packet_loop_callback loop1{peer_api1, peer_api2};
  packet_loop_callback loop2{peer_api2, peer_api1};

  channel_establishment_callback ccb1;

  data_available_callback dcb1;
  data_available_callback dcb2;

  using namespace std::placeholders;

  peer_api1 = new api_t{
    ctx1,
    [](channeler::error_t, channelid) {},
    std::bind(&packet_loop_callback::packet_to_send, &loop1, _1),
    std::bind(&data_available_callback::callback, &dcb1, _1, _2)
  };
  peer_api2 = new api_t{
    ctx2,
    [](channeler::error_t, channelid) {},
    std::bind(&packet_loop_callback::packet_to_send, &loop2, _1),
    std::bind(&data_available_callback::callback, &dcb2, _1, _2)
  };

  // *** Establish channel
  auto err = peer_api1->establish_channel(
    ctx2.node().id(),
    std::bind(&channel_establishment_callback::callback, &ccb1, _1, _2)
  );
  EXPECT_EQ(ERR_SUCCESS, err);

  // *** Send data from peer1 to peer2
  test_data_exchange(ccb1.m_id, "Test #1", *peer_api1,
      dcb2, *peer_api2);

  // *** Send data from peer2 to peer1
  test_data_exchange(ccb1.m_id, "Test #2", *peer_api2,
      dcb1, *peer_api1);

  delete peer_api1;
  delete peer_api2;
}
