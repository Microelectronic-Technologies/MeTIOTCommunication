# Guide - How To Use the Library

This guide provides practical steps for using the MeT IoT Python client library. For details on the underlying packet structure, see [TCP_PROTOCOL_SPEC.md](TCP_PROTOCOL_SPEC.md).

## Installation

The MeT IoT library is *soon to be* available on PyPI.

```bash
pip install x
```

## Device Discovery

MeT IoT devices broadcast their presence using Zeroconf (mDNS). The library includes a function to listen for and find available devices on your local network.

1. **WIP**

## Connecting to a Device

Establish a persistent TCP connection and perform the initial device identification handshake.

1. **Create Client:** Create a client with the devices IP & Port.
    ```py
    import MeTIOT

    client = MeTIOT.DeviceClient("192.168.1.100", 12345)
    ```
2. **Connect:** Use the `connect()` method to connect to the device.
    ```py
    client.connect()
    ```
    **Behind the Scenes:** After connecting to the TCP socket, the client sends a Device Identification Request and waits for the Device Identifier response. This identifies the device type and its unique ID.

3. **Device Protocol:** For sending and interpreting message you will need a protocol handler.
    ```py
    protocol = client.get_protocol_handler()
    ```
    **Behind the Scenes:** After the connection has happened and the device type is identified that devices unique protocol class will be assigned to the client. This protocol class comes with its own unique methods. You can find all unique device protocol methods in the [API docs](API_REFERENCE.md).

## Receiving Data

This library uses a dedicated thread to listen for unsolicited data transfers from the device. You should register callback functions to handle incoming data updates.

> [!NOTE]
> **Prerequisite:** You must first follow [Connecting to a Device](#connecting-to-a-device) creating a client, connecting, **and** fetching the device protocol.

1. **Define a Handler:** Create a function to process the incoming data object.
    ```py
    def unique_device_update_handler(packet):
        print("Received Packet")
        # 1. Use the protocol handler to strip protocol overhead and extract header/data
        header, data = protocol.deconstruct_packet(packet)

        match header:
            case MalformedPacketNotification:
                # Example: If a malformed packet is received (Header 0xFF), re-send most recent packet.
                newPacket = protocol.fetch_last_packet()
                client.send_packet(newPacket)
            case DataTransfer:
                # 2. Use the protocol handler to interpret the Data Payload (Sub-Headers) into a dictionary
                dataDict = protocol.interpret_data(data)
            case _:
                # Handle other unique device commands as defined by the protocol
                pass
    ```

2. **Register the Handler:** Attach the handler to the client instance.
    ```py
    client.on_data_received(unique_device_update_handler)
    ```

3. **Start Listening:** The client will automatically start a background thread to listen for messages.

## Sending Commands

To send a command (like changing a setting), use the client objects protocol handler for high-level command methods.

> [!NOTE]
> **Prerequisite:** You must first follow [Connecting to a Device](#connecting-to-a-device) creating a client, connecting, **and** fetching the device protocol.

1. **Create Packet:** Use the device-specific methods on the `protocol` object to generate the complete, ready-to-send packet buffer.
    ```py
    # Example for sending a calibration command to a Fish Tank device
    # The protocol method generates the Data Payload + Header + CRC + COBS.
    packet = protocol.create_fish_tank_calibration()
    ```

2. **Send Packet:** Supply that packet buffer to the client's send function.
    ```py
    client.send_packet(packet)
    print("Calibration request packet sent.")
    ```
    **Behind the Scenes:** The `protocol` method handles the specific command, including any required Sub-Headers and Data. It then automatically adds the standard protocol overhead (CRC and COBS), returning a complete, byte-stuffed buffer ready for transmission. The client then sends this buffer over the TCP socket.
