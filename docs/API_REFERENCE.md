# API Reference

The `MeTIOT` library provides a high-level interface for interacting with MeTIOT IoT devices via TCP. The core of the library is the `DeviceClient`, which manages the connection, event loops, and protocol handling.

## 1. Client Library Components (`MeTIOT`)

The `DeviceClient` class is the primary entry point for all device interactions.

### Lifecycle Management

#### `DeviceClient(ip: str, port: int)`

*Constructor* | **Module:** `MeTIOT`

Initializes the client object with the target device's network information.

* **Arguments**
    * `ip` (str): IPv4 address of the device.
    * `port` (int): Communication port.
* **Errors:** Raises `SocketError` if the IP address is invalid or unsupported.

#### `connect()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Establishes the TCP connection and performs initial handshake/device discovery.

* **Errors:** Raises `SocketError` if the device is unreachable.

#### `disconnect()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Safely closes the active TCP connection.

---

### Protocol & Identity

#### `get_protocol_handler()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Returns the `ProtocolHandler` specific to the current device type. use this to encode/decode bytes.

* **Returns:** `ProtocolHandler`

> [!TIP]
> Use this instead of the deprecated `get_specific_protocol_handler()`

#### `get_unique_id()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Retrieves the globally unique 64-bit ID assigned to the hardware.

* **Returns:** 8-byte Device ID.

#### `get_device_type()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Identifies the model/type of the connected device.

* **Returns:** `MeTIOT.DeviceType`

---

### Asynchronous Operations

#### `assign_handlers(on_data, on_warning=None, on_fatal=None)`

*Method* | **Module:** `MeTIOT.DeviceClient`

Registers callback functions for the background listening thread.

| Argument | Signature | Description |
|-|-|-|
| `on_data` | `(device, header, data)` | Triggered on successful packet reception. |
| `on_warning` | `(device, msg)` | **Optional.** Triggered on non-fatal issues (e.g., CRC mismatch). |
| `on_fatal` | `(device, msg)` | **Optional.** Triggered when the loop crashes or 10 warnings occur |

#### `start_listening()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Spawns a background thread to monitor the TCP socket. **Requires handlers to be assigned first.**

* **Errors:** `LibraryError` if `assign_handlers` has not been called or if a listening thread is already running.

#### `send_packet(packet: bytes)`

*Method* | **Module:** `MeTIOT.DeviceClient`

Sends a raw byte buffer to the device.

* **Arguments:** `packet` (bytes): The encoded payload.
* **Errors:** `SocketError` on transmission failure.

## 2. Protocol Handler Methods (Accessed via `client.get_protocol_handler()`)

The methods available on the protocol handler instance depend entirely on the connected device type. The protocol handler also contains universal utility methods for packet handling.

### 2.1 Universal Protocol Utility Methods

| Method | Returns | Description |
|-|-|-|
| `deconstruct_packet(packet: bytes)` | `tuple[int, bytes]` | Decodes the packet buffer, validating CRC, performing COBS unstuffing, and returning the Data Header and raw Data Payload. **NOTE:** This function should not be used when the async `on_data` function you declare is called as the data parsed is already decoded. |
| `interpret_data(data_payload: bytes)` | `dict` | Interprets the raw Data Payload into a key-value dictionary using known Sub-Headers and Data Types. Keys match the Data Field purpose. **NOTE:** Refer to your devices [Protocol Constants](#3-protocol-constants-metiotprotocolconstants) for the key & value pairs. |
| `fetch_last_packet()` | `bytes` | Returns the raw byte buffer of the last packet successfully created by this protocol handler instance. Useful for re-sending after a `MalformedPacketNotification`. |

### 2.2 Tank Guardian Protocol Methods (`Device ID: 0xFF`)

These methods are available for devices with the Tank Guardian device type.

| Method | Returns | Purpose | Arguments |
|-|-|-|-|

### 2.3 Filter Guardian Protocol Methods (`Device ID: 0xFE`)

These methods are available for devices with the Filter Guardian device type.

| Method | Returns | Purpose | Arguments |
|-|-|-|-|

## 3. Protocol Constants (`MeTIOT.ProtocolConstants`)

These constants are integers representing the 1-byte Header/Sub-Header codes used in the communication. All constants mentioned here are the exposed constants. In other documentation the codes here are referenced as *Device-to-Library Codes*.

### 3.1 Standard Node Header Codes

| Constant Name | Value | Description |
|:---|:---|:---|
| `MalformedPacketNotification` | `0xFF` | Notification for a corrupted/unparsable packet. |
| `DeviceIdentifier` | `0xFE` | Response from the device containing the Device ID and Unique ID. |
| `DataTransfer` | `0xFD` | Unsolicited data/status update from the device. |

### 3.2 Tank Guardian

#### 3.2.1 Tank Guardian Node Header Codes

| Constant Name | Value | Description |
|-|-|-|

#### 3.2.2 Tank Guardian Interpreted Data Key & Value Pairs

| Key | Value Type | Value Desc | Note |
|-|-|-|-|
| `"Temperature_C"` | Celcius | Temperature sensor temperature | 2 decimal points of percision |
*See [Universal Protocol Utility Methods](#21-universal-protocol-utility-methods) for more information.*

### 3.3 Filter Guardian

#### 3.3.1 Filter Guardian Node Header Codes

| Constant Name | Value | Description |
|-|-|-|

#### 3.3.2 Filter Guardian Interpreted Data Key & Value Pairs

| Key | Value Type | Value Desc | Note |
|-|-|-|-|
| `"Flowrate_LM"` | Litres per minute | Water flow rate sensor reading | 2 decimal points of precision |
| `"Pressure_PSI"` | PSI | Air pressure sensor reading | 3 decimal points of precision |
*See [Universal Protocol Utility Methods](#21-universal-protocol-utility-methods) for more information.*

### 3.4 How To Use These Values?

In your python code you cannot directly compare the function integer parameter with the enum so you must use the value `MeTIOT.ProtocolConstants.Path.To.Constant.value`.

#### 3.4.1 Example

```python
match header:
    case MeTIOT.NodeHeader.General.Data.value:
        deviceProtocol = device.get_protocol_handler()
        telemetry = deviceProtocol.interpret_data(data)
        print(f"Flow rate: {telemetry['Flowrate_LM']} L/m")

    case MeTIOT.NodeHeader.General.MalformedPacket.value:
        print("Device reported a communication error.")

    case _:
        print(f"Unhandled header occured: {header}")
```

## 4. Asynchronous Error Codes (`MeTIOT.AsyncErrorCode`)

These constants are intended for use in the asynchronous warning and error handling functions for a device. These constants represent an integer value defined below but best practice dictates directly using the declared enum **not** the value.

### 4.1 Codes

| Type | Name | Path | Value |
|-|-|-|-|
| Error Only | Timeout/Socket Innactive | `MeTIOT.AsyncErrorCode.TIMEOUT_OR_INACTIVITY` | 100 |
| Error Only | Socket Failure | `MeTIOT.AsyncErrorCode.SOCKET_FAILURE` | 101 |
| Warning & Error | Library Error (Can be either ProtocolError OR EncodingError) | `MeTIOT.AsyncErrorCode.LIBRARY_FATAL` | 200 |
| Error Only | Unexpected System Error | `MeTIOT.AsyncErrorCode.UNEXPECTED_SYSTEM_ERR` | 999 |

> [!NOTE]
> To access these values for use in comparisons with integers add `.value` to the end.
>
> The value of these codes is **NOT** guarenteed to be accurate. Best practice for using these values is defined below in [4.2 How To Use These Values?](#42-how-to-use-these-values).

### 4.2 How To Use These Values?

In your python code you cannot directly compare the function integer parameter with the enum so you must use the value `MeTIOT.AsyncErrorCode.Path.To.Code.value`.

#### 4.2.1 Example

```python
def handle_fatal_error(device, error_code: int, message: str):
    print(f"Device {device.get_unique_id()} has fatal error (code {code}): {msg}")

    if error_code == MeTIOT.AsyncErrorCode.TIMEOUT_OR_INACTIVITY.value:
        print("Device times out. Triggering integration reload")
        reconnect_to_device(device)

    elif error_code == MeTIOT.AsyncErrorCode.SOCKET_FAILURE.value:
        print("Socket connection lost. Reloading...")
        reconnect_to_device(device)
```