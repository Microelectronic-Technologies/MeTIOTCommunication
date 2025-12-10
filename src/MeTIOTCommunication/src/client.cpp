#include "../include/client.hpp"

bool DeviceClient::connect() {
    int ret;

    // Connect to device
    ret = socketCore.connectDevice();

    // Complete Device initialization
    ret = perform_device_initialization();

    return true;
}

void DeviceClient::disconnect() {
    socketCore.disconnect();
}

bool DeviceClient::send_packet(const std::vector<uint8_t>& packet) {
    int bytesSent = socketCore.send_data(packet);
    if (bytesSent != packet.size()) {
        // TODO: Handle not all bytes sent
        return false;
    }

    return true;
}

bool DeviceClient::perform_device_initialization() {
    int err;

    // Perform ECDH
    err = perform_ecdh();

    // Send device identification request


    // Recieve device data


    // Assign specific protocol to protocolHandler (possible push to helper function)

}

bool DeviceClient::perform_ecdh() {
    // TODO: Implement
    
    return true;
}