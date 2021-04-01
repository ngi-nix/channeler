# Channeler

This library provides a reference implementation for a multi-channel and -link
protocol for peer-to-peer communications.

The rationale for this has several dimensions:

* NAT-piercing is prone to failure. Additionally, the number of available ports
  on a NAT limits how many peers behind the NAT can be served. To compensate
  for this, a multi-channel approach effectively allows multiplexing of
  independent "connections" (aka channels) along the same port.
* In a [multi-link](TODO link to paper when available) (or multi-home) device,
  e.g. on mobile devices, application connections should be kept stable even
  when the link technology changes (e.g. from WiFi to LTE, etc.)
* Finally, encryption parameters can be kept separate per channel, improving
  recovery times, when the encryption protocol is aware of both of the above.

The library is implemented with readability and extensibility in mind. Other
implementations may well opt for stronger optimization instead.

**Note:** the library is under heavy development, and the README will be updated when
  the first stab is implemented.

For more details on the protocol design, [Connection Reset by Peer](https://reset.substack.com)
has blog posts on the design rationale. Additionally, the
[architecture overview](docs/architecture.md) contains the rationale for the
pipe-and-filter approach chosen in the protocol implementation.

# Status

This repository is *heavily* work-in-progress. Currently implemented is:

- [x] Channel negotiation
- [ ] Resend/reliability features
- [ ] Encryption
- [ ] Mult-Link capabilities
  - [ ] connection management
- [ ] finalized API

# Usage

The current API is for internal use only. It does provide the main parts for
verifying the protocol logic.

The following examples are similar to the `InternalAPI` test suite.

```c++
// A transport address type; this one is enough for IPv4
using address = uint32_t;

// How much memory should be allocated in one go in the pool? This is
// a multiple of *packets*.
constexpr std::size_t POOL_BLOCK_SIZE = 20;

// How large are packets? This is currently static, and should be chosen to
// fit the path MTU.
constexpr std::size_t PACKET_SIZE = 1500;

// The node context contains the local peer identifier, and other per-node
// data.
using node = ::channeler::context::node<POOL_BLOCK_SIZE>;

// The connection context contains per-connection data, e.g. the number of
// registered channels, etc.
using connection = ::channeler::context::connection<address, node>;

// Internal API instance
using api = ::channeler::internal::connection_api<connection>;
```

With these types and constants defined, we can create an API instance:

```c++
// Node information
::channeler::peerid self;

node self_node{
  self,
  PACKET_SIZE,
  // A callback returning std::vector<std::byte>; this is a secret used
  // for cookie generation.
  &secret_callback,
  // The sleep function should accept a duration to sleep for, and return
  // the duration actually slept for.
  &sleep_function
};

// Connection from self to peer
::channeler::peerid peer;
connection conn{self_node, peer};

// API instance
api conn_api{
  conn,
  // The callback is invoked when a channel is established.
  &channel_established_callback,
  // The callback is invoked when the API has produced a packet that should be
  // sent to the peer.
  &packet_available_callback,
  // The last callback is invoked when there is data to read from a channel.
  &data_to_read_callback
};
```

First, we need to establish a channel.

```c++
auto err = conn_api.establish_channel(peer);
```

The callback when a packet is available is going to be invoked.

```c++
void packet_available_callback(channeler::channeld const & id)
{
  // Read the packet from the API instance.
  auto packet = conn_api->packet_to_send(id);

  // Write the packet to the I/O, e.g. a socket.
  write(sockfd, entry.packet.buffer(), entry.packet.buffer_size());
}
```

When the peer responds, the channel establishment callback is going to be
invoked (skipped here). You can now write to the channel.

```c++
channelid id; // from callback

size_t written = 0;
auto err = conn_api.write(id, message.c_str(), message.size(), written);

assert(written == message.size());
```

You can create many channels per connection, and each channel is handled
separately. When reliability features are implemented, this means that
packet loss on one channel will not stall packets on other channels.
