#include "../include/client.hpp"

bool DeviceClient::connect() {

    // Connect to device
    std::cout << "Skipping the actual socket connection\n";
    //int ret = socketCore.connectDevice();

    // Complete Device initialization
    bool success = perform_device_initialization();

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

bool DeviceClient::assign_device_protocol(uint8_t deviceID, std::vector<uint8_t>& key) {
    switch(deviceID) {
        case (static_cast<uint8_t>(Protocol::DeviceIdentifier::FishTank)): {
            deviceType = DeviceType::FISH_TANK;
            protocolHandler = std::make_unique<FishTankProtocol>(key);

            return true;
        }
        default: {
            DeviceType::UNKNOWN;

            return false;
        }
    }
}

bool DeviceClient::perform_device_initialization() {
    bool success;
    
    // -- Perform ECDH
    std::vector<uint8_t> key;
    success = perform_ecdh(key);
    std::cout << "perform_ecdh ret: " << (success ? "True" : "False") << std::endl;

    // -- Send device identification request
    // Create DeviceAgnosticProtocol
    std::cout << "Creating DeviceAgnosticProtocol\n";
    DeviceAgnosticProtocol deviceAgnosticProtocol(key);

    std::cout << "Creating id req packet\n";
    std::vector<uint8_t> packet = deviceAgnosticProtocol.create_device_id_request();
    
    // Send data
    std::cout << "Skipping send_packet function\n"; 
    //success = send_packet(packet);
    if (!success) {
        // TODO: Error
        std::cout << "Failed but skipping failure return\n";
        //return false;
    }

    // -- Recieve device data



    // -- Assign specific protocol to protocolHandler (possible push to helper function)
    uint8_t deviceID = 0xFF;
    success = assign_device_protocol(deviceID, key);
    if (!success) {
        std::cout << "No success";
    }
    
    return true;
}

bool DeviceClient::perform_ecdh(std::vector<uint8_t>& key) {
    // TODO: Implement
    key.resize(32);

    std::cout << "perform_ecdh test... Key value: ";
    for (int i = 0; i < 32; i++) {
        key[i] = i;
        std::cout << i << " ";
    }
    std::cout << std::endl;
    
    return true;
}