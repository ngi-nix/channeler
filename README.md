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
