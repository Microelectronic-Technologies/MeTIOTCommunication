# Guide - How To Use the Library

When moving through this guide underneath the steps that are declared will always be a section stating **what** is being done behind the scenes during this process. For all normal use these section can be completely ignored (unless you're interested).

## Before Connection

### Device Discovery

While not supported directly in this library all MeT IoT devices supported in this library broadcast their presence using [zeroconf](https://en.wikipedia.org/wiki/Zero-configuration_networking). Once a device is discovered the steps below in the `Start Device Communication` should be followed.

### Start Device Communication

1. **WIP**



## After Connection

### Receiving Messages

**WIP** *Use Thread for blocking function*

### Sending Messages

1. Get the devices protocolHandler with `get_protocol_handler()`.
2. Generate a packet into a buffer using a function supplied through the devices protocolHandler (These exact functions are device specific).
3. Supply that packet to `send_packet` function.

The above specifies the intended method when sending a message. Using these exact steps will automatically handle the following:

* Data creation (header, data, etc).
* Cyclic Redundancy Check creation.
* Random IV generation.
* AES-256 Data Encryption.
* COBS encoding.