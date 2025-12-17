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

1. **Create Client:** Create a client with the device's IP & Port.
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
    **Behind the Scenes:** After the connection has happened and the device type is identified that devices unique protocol class will be assigned to the client. This protocol class comes with its own unique methods. You can find all unique device protocol methods in the [API Reference](API_REFERENCE.md).

## Receiving Data

This library uses a dedicated thread to listen for unsolicited data transfers from the device. You should register callback functions to handle incoming data updates.

> [!NOTE]
> **Prerequisite:** You must first follow [Connecting to a Device](#connecting-to-a-device) creating a client, connecting, **and** fetching the device protocol.

1. **Define a Handler:** Create a function to process the incoming data object. You will likely have different functions for each device type you expect to handle.
    ```py
    def my_update_handler(header, data):
        print(f"Received message with header: {header}")

        match header:
            case MeTIOT.Header.DataTransfer:
                # Use the protocol handler to interpret the raw bytes into a dictionary
                telemetry = protocol.interpret_data(data)
                print(f"Temperature: {telemetry['Temperature_C']} C")

            case MeTIOT.Header.MalformedPacketNotification:
                print("Device reported a communication error.")
                # Since the library is stateless, you must track your 
                # last sent packet locally if you wish to re-send it.

            case _:
                # Handle other commands defined in the API Reference
                pass
    ```

2. **Register the Handler:** Attach the handler to the client instance.
    ```py
    # --- To assign all devices the same handler ---
    client.on_data_received(my_update_handler)

    #   OR
    # --- To assign different devices types unique handlers ---
    #
    # devType = client.get_device_type() # Fetch the current device type
    #
    # if (devType == MeTIOT.DeviceType.FISH_TANK):
    #     client.on_data_received(fish_tank_update_handler)
    # elif (devType == MeTIOT.DeviceType.UNKNOWN):
    #     print("Unknown device type. Assigning generic handler")
    #     client.on_data_received(generic_update_handler)
    ```
    **Behind the Scenes:** The client initiates a background listener thread to monitor the socket. When a message is detected, the library automatically handles the decoding and verification (CRC/COBS). It segregated the payload into its `header` and `data` components and dispatches them directly to your handler.

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
    **Behind the Scenes:** The protocol handler wraps your command with mandatory overhead (CRC and COBS encoding), ensuring the device can validate the integrity of the message upon receipt.
