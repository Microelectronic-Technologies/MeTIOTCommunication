# API Reference

This documentation provides a comprehensive list of all public methods, properties, and constants exposed by the MeT IoT Python client library (`MeTIOT`).

## 1. Client Library Components (`MeTIOT`)

### 1.1 `MeTIOT.DeviceClient` (Main Class)

| Property/Method | Type | Description |
|-|-|-|
| `__init__(ip: str, port: int)` | Constructor | Initializes the client. |
| `connect()` | Method | Establishes the TCP connection and performs the device initialization. |
| `disconnect()` | Method | Closes the TCP connection. |
| `on_data_received(handler: callable)` | Method | Register a callback function to handle raw incoming packet buffers. Handler signature: `handler(packet: bytes)`. |
| `get_protocol_handler()` | Method | **Returns the device-specific protocol handler instance.** Must be called after `connect()`. |
| `send_packet(packet: bytes)` | Method | Sends a pre-constructed packet buffer over the TCP connection. |

## 2. Protocol Handler Methods (Accessed via `client.get_protocol_handler()`)

The methods available on the protocol handler instance depend entirely on the connected device type. The protocol handler also contains universal utility methods for packet handling.

### 2.1 Universal Protocol Utility Methods

| Method | Returns | Description |
|-|-|-|
| `deconstruct_packet(packet: bytes)` | `tuple[int, bytes]` | Decodes the packet buffer, validating CRC, performing COBS unstuffing, and returning the Data Header and raw Data Payload. |
| `interpret_data(data_payload: bytes)` | `dict` | Interprets the raw Data Payload into a key-value dictionary using known Sub-Headers and Data Types. Keys match the Data Field purpose (e.g., "Temperature_C"). |
| `fetch_last_packet()` | `bytes` | Returns the raw byte buffer of the last packet successfully created by this protocol handler instance. Useful for re-sending after a `MalformedPacketNotification`. |

### 2.2 Fish Tank Protocol Methods (`Device ID: 0xFF`)

These methods are available if the device type is "Fish Tank"

| Method | Returns | Purpose | Arguments |
|-|-|-|-|

## 3. Protocol Constants (`MeTIOT.ProtocolConstants`)

These constants are integers representing the 1-byte Header/Sub-Header codes used in the communication. All constants mentioned here are the exposed constants. In other documentation the codes here are referenced as *Device-to-Library Codes*.

### 3.1 Standard Data Header Codes

| Constant Name | Value | Description |
|:---|:---|:---|
| `MalformedPacketNotification` | `0xFF` | Notification for a corrupted/unparsable packet. |
| `DeviceIdentifier` | `0xFE` | Response from the device containing the Device ID and Unique ID. |
| `DataTransfer` | `0xFD` | Unsolicited data/status update from the device. |

### 3.2 Fish Tank Data Header Codes

| Constant Name | Value | Description |
|-|-|-|