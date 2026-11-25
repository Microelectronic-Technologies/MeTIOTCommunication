# API Reference

The `MeTIOT` library provides a high-level interface for interacting with MeTIOT IoT devices via TCP. The core of the library is the `DeviceClient`, which manages connections, event loops, and protocol handling.

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

Establishes the TCP connection and performs the initial handshake/device discovery.

* **Errors:** Raises `SocketError` if the device is unreachable.

#### `disconnect()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Safely closes the active TCP connection.

---

### Protocol & Identity

#### `get_protocol_handler()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Returns an instance of `AbstractProtocol` specific to the current device type. Use this to encode or decode bytes.

* **Returns:** `AbstractProtocol`

> [!TIP]
> Use this instead of the deprecated `get_specific_protocol_handler()`.

#### `get_unique_id()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Retrieves the globally unique 64-bit ID assigned to the hardware.

* **Returns:** 8-byte Device ID (int).

#### `get_device_type()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Identifies the model/type of the connected device.

* **Returns:** `MeTIOT.DeviceType` enum.

---

### Asynchronous Operations

#### `assign_handlers(on_data, on_warning=None, on_fatal=None)`

*Method* | **Module:** `MeTIOT.DeviceClient`

Registers callback functions for the background listening thread.

| Argument | Signature | Description |
| :--- | :--- | :--- |
| `on_data` | `(device, header, data)` | Triggered on successful packet reception. |
| `on_warning` | `(device, msg)` | **Optional.** Triggered on non-fatal issues (e.g., CRC mismatch). |
| `on_fatal` | `(device, msg)` | **Optional.** Triggered when the loop crashes or 10 warnings occur. |

#### `start_listening()`

*Method* | **Module:** `MeTIOT.DeviceClient`

Spawns a background thread to monitor the TCP socket. **Requires handlers to be assigned first.**

* **Errors:** Raises `LibraryError` if `assign_handlers` has not been called or if a listening thread is already running.

#### `send_packet(packet: bytes)`

*Method* | **Module:** `MeTIOT.DeviceClient`

Sends a raw byte buffer to the device.

* **Arguments:** `packet` (bytes): The encoded payload.
* **Errors:** Raises `SocketError` on transmission failure.

## 2. Protocol Handler Methods

*(Accessed via `client.get_protocol_handler()`)*

The methods available on the protocol handler instance depend on the connected device type.

### 2.1 Universal Protocol Utility Methods

| Method | Returns | Description |
| :--- | :--- | :--- |
| `deconstruct_packet(packet: bytes)` | `tuple[int, bytes]` | Decodes the packet buffer, validates CRC, and performs COBS unstuffing. **Returns (Data Header, Raw Payload).** <br><br> ⚠️ **NOTE:** Do not use this if you are using the async `on_data` handler, as the data is already decoded by the time it reaches your callback. |
| `interpret_data(data_payload: bytes)` | `dict` | Interprets the raw Data Payload into a key-value dictionary using known Sub-Headers. <br><br> ⚠️ **NOTE:** Refer to [Protocol Constants](#3-protocol-constants) for valid keys and values. |
| `fetch_last_packet()` | `bytes` | Returns the raw byte buffer of the last packet successfully created by this protocol handler. Useful for re-sending after a `MalformedPacketNotification`. |

### 2.2 Tank Guardian Protocol Methods (`Device ID: 0xFF`)

*(Methods specific to Tank Guardian will be listed here as they're added)*

### 2.3 Filter Guardian Protocol Methods (`Device ID: 0xFE`)

*(Methods specific to Filter Guardian will be listed here as they're added)*

## 3. Protocol Constants (`MeTIOT.ProtocolConstants`)

These constants represent the 1-byte Header/Sub-Header codes used in communication. In other documentation, these are referred to as *Device-to-Library Codes*.

### 3.1 Standard Node Header Codes

| Constant Name | Value | Description |
| :--- | :--- | :--- |
| `MalformedPacketNotification` | `0xFF` | Notification for a corrupted/unparsable packet. |
| `DeviceIdentifier` | `0xFE` | Response from the device containing the Device ID and Unique ID. |
| `DataTransfer` | `0xFD` | Unsolicited data/status update from the device. |

### 3.2 Tank Guardian

#### 3.2.1 Tank Guardian Node Header Codes

*(Constants will be listed here)*

#### 3.2.2 Tank Guardian Interpreted Data Key & Value Pairs

| Key | Value Type | Description | Note |
| :--- | :--- | :--- | :--- |
| `"Temperature_C"` | `float` | Temperature sensor reading | 2 decimal points of precision |

### 3.3 Filter Guardian

#### 3.3.1 Filter Guardian Node Header Codes

*(Constants will be listed here)*

#### 3.3.2 Filter Guardian Interpreted Data Key & Value Pairs

| Key | Value Type | Description | Note |
| :--- | :--- | :--- | :--- |
| `"Flowrate_LM"` | `float` | Water flow rate sensor reading | 2 decimal points of precision |
| `"Pressure_PSI"` | `float` | Air pressure sensor reading | 3 decimal points of precision |

### 3.4 Usage Pattern: Comparing Headers

In Python, you cannot directly compare an integer parameter with an Enum object. You must compare the integer against the `.value` attribute of the constant.

#### Example

```python
match header:
    case MeTIOT.NodeHeader.General.Data.value:
        device_protocol = device.get_protocol_handler()
        telemetry = device_protocol.interpret_data(data)
        print(f"Flow rate: {telemetry['Flowrate_LM']} L/m")

    case MeTIOT.NodeHeader.General.MalformedPacket.value:
        print("Device reported a communication error.")

    case _:
        print(f"Unhandled header occurred: {header}")
```

## 4. Asynchronous Error Codes (`MeTIOT.AsyncErrorCode`)

These constants are used within the asynchronous warning and error handling functions. **Best practice dictates using the Enum member directly, not its integer value.**

### 4.1 Codes

| Type | Name | Path | Value |
|-|-|-|-|
| Error Only | Timeout/Inactivity | `MeTIOT.AsyncErrorCode.TIMEOUT_OR_INACTIVITY` | 100 |
| Error Only | Socket Failure | `MeTIOT.AsyncErrorCode.SOCKET_FAILURE` | 101 |
| Warning & Error | Library Fatal Error | `MeTIOT.AsyncErrorCode.LIBRARY_FATAL` | 200 |
| Error Only | Unexpected System Error | `MeTIOT.AsyncErrorCode.UNEXPECTED_SYSTEM_ERR` | 999 |

> [!NOTE]
> To access these values for use in integer comparisons, append `.value` to the end of the constant.

### 4.2 Usage Example

```py
def handle_fatal_error(device, error_code: int, message: str):
    print(f"Device {device.get_unique_id()} has fatal error (code {error_code}): {message}")

    if error_code == MeTIOT.AsyncErrorCode.TIMEOUT_OR_INACTIVITY.value:
        print("Device timed out. Triggering integration reload...")
        reconnect_to_device(device)

    elif error_code == MeTIOT.AsyncErrorCode.SOCKET_FAILURE.value:
        print("Socket connection lost. Reloading...")
        reconnect_to_device(device)
```
