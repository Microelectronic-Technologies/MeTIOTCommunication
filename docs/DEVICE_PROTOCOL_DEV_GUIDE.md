# Device Protocol Development Guide

This guide is intended for contributors adding support for new MeT IoT devices.

## Architectural Overview

### The Abstraction Challenge

The `DeviceClient` class utilizes a `std::unique_ptr<AbstractProtocol>` to manage communication. While this provides a clean interface for general data transfer, it creates a type-erasure problem: the base `AbstractProtocol` does not contain methods specific to individual device types (e.g., specialized calibration routines for a Fish Tank).

### The Implementation Strategy

To maintain a unified API for Python users, we do not expose the underlying type complexity. Instead, when a device-specific command is requested via Python:

1. The system identifies the active `DeviceType`.
2. The C++ backend performs a `dynamic_cast` from `AbstractProtocol` to the specific implementation (e.g., `FishTankProtocol`).
3. The specific method is executed and the result is returned to the Python layer.

## Workflow: Adding a New Device Protocol

Follow these steps to implement support for a new device type:

1. **File Creation:**
    - Add a header file (`.hpp`) to `include/protocol/`.
    - Add an implementation file (`.cpp`) to `src/protocol/`.
2. **Class Implementation:** Create a new class that inherits from `AbstractProtocol`. Ensure you implement all required virtual functions, such as `constructPacket()`.
3. **Reference Pattern:** Follow the logic pattern used in existing protocols: `Receive Request` -> `Construct Data Field` -> `Call constructPacket()` -> `Return Packet`.
4. **Type Registration:** Add your new device type to the `enum class DeviceType` located in `include/client.hpp`.
5. **Client Integration:** Update the device identification logic in `src/client.cpp` to recognize and instantiate your new protocol class.
6. **Python Bindings:** Update `/src/python_bindings/bindings.cpp` to:
    - Expose new device-specific functions to Python.
    - Implement the `dynamic_cast` logic required to bridge the Python call to your specific protocol class.

> [!TIP]
> **Constants:** If you need to add more outgoing fields, command headers, or sub-header names, add them to `include/protocol/protocol_constants.hpp`.
