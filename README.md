# MeT IOT Communication

A C++23 communication library with Python bindings for Microelectronic Technologies IoT devices.

## Useful Links

- [API Reference](https://github.com/Microelectronic-Technologies/MeTIOTCommunication/blob/main/docs/API_REFERENCE.md)
- [Python Library Guide](https://github.com/Microelectronic-Technologies/MeTIOTCommunication/blob/main/docs/PYTHON_LIB_GUIDE.md)
- [Example Projects](https://github.com/Microelectronic-Technologies/MeTIOTCommunication/tree/main/examples)

## Changelog `v0.1.12`

- **Resolved Python 3.14 Exit Crash (`SIGABRT` / `free(): invalid pointer`):** Upgraded `pybind11` to `v3.1.0`, removed the invalid module cleanup capsule, and fixed callback teardown in `PythonEventHandler`.
- **Thread & GIL Safety:** Replaced the detached listener thread with a joinable `std::jthread` in `DeviceClient` and added `gil_scoped_release` guards around blocking socket I/O calls.
- **Testing & CI:** Added C++ unit/integration tests (`Catch2`), Python extension tests (`pytest`), and a GitHub Actions CI workflow covering C++ builds and Python 3.12–3.14.

## How To Use

### Installation

Install prebuilt wheels via `pip` (supports Python 3.12+):

```sh
pip install MeTIOT
```

### Quick Start

Verify the library imports and initializes properly:

```py
import MeTIOT

client = MeTIOT.DeviceClient("0.0.0.0", 12345)

print(type(client))
print("MeTIOT import successful!")
```

For full usage instructions, async callbacks, and protocol details, refer to the [Python Library Guide.](https://github.com/Microelectronic-Technologies/MeTIOTCommunication/blob/main/docs/PYTHON_LIB_GUIDE.md)

## Building from Source

### Python Package (Recommended)

1. Clone the repository:

    ```bash
    git clone https://github.com/Microelectronic-Technologies/MeTIOTCommunication.git
    cd MeTIOTCommunication
    ```

2. Install in editable or standard mode (requires a C++23 compiler and CMake):

    ```bash
    pip install .
    ```

### C++ Library & Tests Only

1. Configure and build with CMake:

    ```bash
    cmake -B build -DBUILD_TESTING=ON -DBUILD_PYTHON_BINDINGS=OFF -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ```

2. Run the C++ test suite:

    ```bash
    ctest --test-dir build --output-on-failure
    ```

### Running Python Tests

```bash
pip install ".[test]"
pytest
```
