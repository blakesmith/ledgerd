# LedgerD

## What is it?

Ledgerd is a messaging system. It is broken into two primary
components:

- libledger - The underlying storage library. Embeddable as a C
  library inside your own process
- ledgerd - a C++ [GRPC](http://grpc.io) interface on top of
libledger, if you'd rather talk to a centralized server process.

Ledger borrows many ideas from other systems, namely
[Apache Kafka](http://kafka.apache.org/),
[jlog](https://labs.omniti.com/labs/jlog) and other open source
messaging systems. Making messaging infrastructure the central
component of your data system is a powerful idea that helps build
scalable data systems and topologies. Ledger aims to be a flexible
tool that can help with that goal.

## What it's not

- No Zookeeper: Ledgerd should aim to be standalone, and avoid
  depedencies that are complicated.
- Scales up and down: You should be able to use the correct messaging
  abstractions early on in your project lifecycle. Many messaging
  tools are reached for too late, only because they add unecessary
  overhead to the development process. I should be able to embed
  ledger inside my process if I want to, or scale ledger up to a large
  distributed infrastructure as my project grows.
- Custom protocols: Ledger attempts to make all interface boundaries
  open and accessible: We use the [GRPC](http://grpc.io) RPC protocol
  to make interactions with the server statically typed, and usable by
  any client that GRPC supports.

## Command Line Client Example

    # ledgerd_client ping

    # ledgerd_client --topic my_topic --partition_count 1 --command open_topic
    => OPENED: my_topic

    # ledgerd_client --topic my_topic --partition 0 --data hello --command write_partition
    => ID: 43
    
    # ledgerd_client --topic my_topic --partition 0 --start 43 --command read_partition --nmessages 1
    => hello
