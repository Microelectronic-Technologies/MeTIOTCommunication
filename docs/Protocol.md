# TCP Communication Protocol Specifications

## Where is this protocol used?

This protocol is used for all TCP communication between MeT IOT devices and external devices (standalone library, Home Assistant, etc.)

## Connection Initialization Sequence

The following protocol outlines the process MeT IOT devices take when starting a connection with this library.

0. Zeroconf packet sent out for device discovery.
1. Library connects to devices TCP socket.
2. Library initiates ECDH handshake (*see `ECDH Handshake` for more details*).
3. Library sends a identification request command.
4. Device responds with its device type and unique identifier.
5. Regular use begins.

## ECDH Handshake

**TODO**

## Encryption

This protocol uses AES-256-CBC. Each message but be encrypted with a new unique 16 byte IV that is transported with the message (*see `Protocol Format`*)

## CRC

**TODO**

G.D. Nguyen, “Fast CRCs,” IEEE Transactions on Computers, vol. 58, no. 10, pp. 1321-1331, Oct. 2009

## Protocol Format

![Packet Diagram](images/Packet.jpg)

### Packet Format

| Field | Description | Bytes |
|-|-|-|
| COBS Header | Used to identify the start of a packet. Stores information on how data after it is to be decoded | 1 |
| Encrypted Data | Data encrypted with AES-256. See `Encrypted Block Format` below. | A multiple of 16 bytes (16, 32, 48, 64, etc.) |
| IV | The IV used during encryption. Ensures each encrypted message is unique even if data is identical. | 16 |
| COBS Trailer | Used to identify the end of a packet. This is always 0x00 | 1 | 

### Encrypted Block Format

| Field | Description | Bytes |
|-|-|-|
| CRC | 16 bit Cyclic Redundancy Check. Used for error detection in the data | 2 |
| Data Header | Identifies packet purpose | 1 |
| Data | Payload as defined below (dependent on direction & Data Header) | 0-n |

### ESP32 to Library Protocol

| Purpose | Header | Data | Bytes |
|-|-|-|-|
| Malformed Packet Notification | 0xFF | | 1 |
| Device Identifier | 0xFE | 1 byte device identifier (*see `Device Identification Codes`*) followed by a 4 byte unique device identifier | 5 |
| Data | 0xFD | Refer to `Device Data Field` |  |

### Library to ESP32 Protocol

| Purpose | Header | Data | Bytes |
|-|-|-|-|
| Malformed Packet Notification | 0xFF | | 1 |
| Device Identification Request | 0xFE | | 1 |

### Device Identification Codes

| Code (1 Byte) | Device |
|-|-|
| 0xFF | Fish Tank |

### Device Data Field

![Device Data Field Diagram](images/DeviceData.jpg)

*As stated in the diagram, this only represents the **Data Header and Data Subheader/Data** sections of the packet format (See `Packet Diagram` above)*

#### Device Data Field Format

| Field | Purpose |
|-|-|
| Data sub-header (*see `Device Data Field Sub-Headers`*) | Identify the data proceeding |
| Data | Data received. To be interpreted using information gathered from the Sub-Header |

> The above pattern repeats for all information the device wishes to transfer: Sub-Header, Data, Sub-Header, Data, ...

#### Device Data Field Sub-Headers

| Sub-Header | Data Field | Data Type |
|-|-|-|
| 0x00 | Data End | |

> [!IMPORTANT]
> Only the following data types are supported:
>
> * `int8_t`
> * `char`
> * `int16_t`
> * `int32_t`
> * `float`
> * `double`
> * `size_t`

## Purpose Of Protocol Components

### COBS

Consistent Overhead Byte Stuffing (COBS) provides this protocol reliable delineate (mark the start and end of) packets, ensuring the entire data stream has been received.

* **Packet Delimitation:** It ensures that the specific byte used as the packet trailer (in this case `0x00`) never appears within the actual packet data (the COBS Header, Encrypted Data, and IV).
* **Decoding Information:** The COBS Header provides essential information for the receiver on how to decode the data, specifically where the original `0x00` bytes (if any) were located before encoding.

### AES-256 Encryption Blocks

The purpose of using AES-256-CBC is to ensure the confidentiality and integrity of the data transmitted over the network.

* **Confidentiality:** All data is scrambled so that only parties processing the correct shared key (establish via the ECDH Handshake) can decrypt and read the message content, preventing eavesdropping.

### IV

The Initialization Vector is a unique 16-byte code used for every message encyption to ensure that even if the same plaintext data is sent multiple tiems, the resulting ciphertext (Encrypted Data) will be different each time.

* **Security:** The IV transforms even the exact same data into unique looking encrypted messaging preventing patterns from being exposed to attackers.
* **Transport:** The IV is transported unecrypted within the Packet Format so the receiving device can use it to correctly initialize the decryption process for that specific message.

### CRC

Cyclic Redundancy Check is a 16-bit code that provides error detection for the message payload.

* **Data Integrity Check:** The sending device calculates a small check value based on the data header and data content. The receiving device recalculates the CRC from the received Data Header and Data and compares it to the transmitted CRC value.
* **Error Detection:** If the two values do not match, it signifies that one or more bits of either the data or CRC have been altered/corrupted during transmission (e.g., due to network noise), and the receiving device can discard the packet as unreliable.