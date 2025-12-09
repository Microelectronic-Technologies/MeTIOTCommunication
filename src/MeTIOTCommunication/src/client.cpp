#include "../include/client.hpp"

bool DeviceClient::connect() {
    int err;

    // Connect to device
    err = socketCore.connect();

    // Complete Device initialization
    err = perform_device_initialization();

    return true;
}

void DeviceClient::disconnect() {

}

bool DeviceClient::send_command(uint8_t command) {

}

bool DeviceClient::send_command(uint8_t command, std::vector<uint8_t>& payload) {

}

bool DeviceClient::perform_device_initialization() {
    int err;

    // Perform ECDH
    err = perform_ecdh();

    // Send device identification request


    // Recieve device data



}

bool DeviceClient::perform_ecdh() {
    // TODO: Implement
    
    return true;
}