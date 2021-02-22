Channeler Component Architecture
================================

I've recently been re-reading Roy Fielding's dissertation on the [REST
architectural style](https://www.ics.uci.edu/~fielding/pubs/dissertation/top.htm),
not to be confused with what people call REST or RESTful, which is usually
different, and often enough the exact opposite of REST.

In the paper, Fielding also describes a pipe-and-filter architectural style.
And while we don't apply this style to how Channeler may be deployed in
a system of systems, it certainly applies well to the arrangement of the
main processing components within the Channeler *software*.

It's also worth stressing that we don't have a universal pipe-and-filter,
which means we cannot compose filters arbitrarily into a pipe. And by only
providing a single filter for each step in the pipe, the composability of
the architectural style is largely theoretical. What we do gain, however,
is a clear delineation of concerns between filter steps.

Pipe-and-filter architectures can be conceived as PUSH or PULL systems.
The main advantage of PULL systems is that data moves through the pipe
based on application demands, which mostly keeps the application simple - it
effectively just iterates through available data, the PULL propagates through
the pipe, and ultimately reads data from the network.

This is, however, not how network I/O operates. Fundamentally, data that
arrives at a network interface *must* be consumed in a timely fashion or be
discarded, as interface buffers are very limited in size. For networking
applications, a PUSH-based pipe-and-filter is much more sensible, which is
why some environments espouse "event"-based programming styles.

However, any PUSH-based system can be turned into a PULL-based application
interface by the introduction of a further buffer, and in the application,
such buffers can be (relatively) unbounded. We therefore provide a PUSH
pipe-and-filter architecture, allowing for a PULL-based message queue to
be added for simplifying the API.

  +----------------------+     +--------+      +----------+
  | PUSH pipe-and-filter |---->| Buffer |<-----| PULL API |
  +----------------------+     +--------+      +----------+

Filter Interface
----------------

Since we do not provide a universal filter interface, the exact way each
interface function is parametrized depends on the position of the filter in
the pipe.

However, the general design of the filter interface, being PUSH-based, is
a function that consumes events, and produces actions. At the end of the
pipe, a filter implements the protocol semantics in a finite state machine.
Such a state machine reacts to incoming events, transitions to new events,
but may require the surrounding system to change. Such change requests are
communicated as actions for the system to take.

We choose the term "action" to differentiate it from events, but also from
the often discussed command pattern.

- `Events` are largely informational messages. They transport information
  that an event occurred, and may contain contextual data, but leave the
  processing of the data up to the recipient.
- `Commands` encapsulate data and logic. They are used for abstracting out
  knowledge about the sender's internals, while allowing the recipient to
  execute commands as part of a larger algorithm.
- `Actions` are effectively events, but carry the implied intent for some
  recipient to execute some algorithm on their receipt.

The main difference to events lies in which party defines their semantics: for
events, it is the sender - it decides *why* it sends an event, and does not
care about whether the event is received at all, or what happens when it is.
With actions, the recipient defines its intent to do something upon receipt,
and the sender decides that this effect is desirable. An action is more like
an API function in this regard.

Actions provide a kind of coupling between filters, but instead of coupling
directly in the composition of the pipe, the coupling is looser via an
interface that filters may or may not implement.

The generic filter interface can therefore be described in pseudo-C++ like
this:

```cpp
  List<IAction> consume(IEvent);
```

Note that this is a fundamentally synchronous operation. Asynchronous
operations can be introduced easily enough, provided that each IEvent
and IAction is self-contained, or at least contains ownership of the data
it references, such that passing IEvent or IAction also passes ownership.

Zero-Copy and Buffering
-----------------------

The ideal system for network performance is one where data packets do not need
to be copied when ownership is passed between filters. Simultaneously, the ideal
system for overall performance is one where memory allocations can be kept to
a minimum. The solution for combining both is using a larger memory pool from
which slots can be reserved; such reservations or ownership can then be passed
from filter to filter with little overhead.

The [Channel Capabilities](https://reset.substack.com/p/channel-capabilities)
article outlines how the different capabilities of channels lead to the need
for per-channel buffers with their own semantics (such as e.g. FIFO). Also,
per-channel buffers may be resized independently.

We address these additional constraints by introducing a two-layer approach:

a) Only a single, central memory pool is used as a buffer. Reservations in the
   pool get transferred to per-channel buffer abstractions.
b) Per-channel buffer abstractions act as a buffer, providing the required
   channel semantics, while never actually owning any memory.

This approach also allows for true zero copy behaviour after the operating
system boundary:

1. networking code reserves a memory slot that is as-yet not associated with
   a channel and passes it down the filter pipe.
1. An initial filter can parse the packet header, and process the packet
   accordingly. In most cases, it will request that the per-channel buffer
   now own the packet. If the buffer management decides the channel buffer is
   full, the packet can be discarded.
1. Further filters pass the reference to the packet which is now associated
   with a channel buffer, until processing ends.
1. Releasing the packet from a channel buffer also releases its slot reservation
   in the memory pool.

Filters
-------

The pipe arranges filters in the following steps:

1. `De-Envelope` - parses the public packet header.
1. `Route` - routes the packet according to header information. Note that we
   currently do not perform routing between e.g. peers, though this could be
   implemented here if a packet was intended for another recipient. Rather, the
   main decision point is whether to further process or drop the packet, e.g.
   according to "firewall" rules.
1. `Validate` - here, we provide simple packet-level validation. The protocol
   identifier must be one of the implemented set, and checksums have to be
   validated. Note that when encryption is used, this may be conflated with the
   decryption filter below.
1. `Decrypt` - only applies if the packet is encrypted. [not yet implemented]
1. `Channel Assignment` - once a packet is decrypted, it can be put into a
    channel specific buffer.
1. `Message Parsing` - this filter processes each packet as a sequence of
   messages, and passes messages further down the filter pipe (at the latest,
   it is here that the universality of the pipe-and-filter architecture is
   broken).
1. `State handling` - this last filter encapsulates the protocol state machine.
   Any internal messages are processed and discarded, while data messages are
   made available to the application.
   This filter provides data and other notifications to the application layer,
   which is considered a "filter" here.

The action part of the interface is in particular concerned with passing
information back from later filters to earlier ones. In particular, it is
interesting for various filters to inform earlier filters that a packet is
not valid, and let earlier filters provide some kind of security against
further malformed packets.

- `State handling` can inform earlier filters of messages that do not fit
  the protocol state and should be considered malformed or malicious.
- `Message parsing` can inform earlier filters of malformed packet payloads.
- `Decrypt` and `Validate` can inform earlier filters that basic packet
  validity could not be ensured.

It is then up to e.g. the `Route` filter, or indeed the layer managing network
I/O to discard further packets from the peer associated with the malformed or
malicious packet.

