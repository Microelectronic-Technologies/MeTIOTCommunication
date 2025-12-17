#include "../include/client.hpp"

void DeviceClient::listen_loop() {
    while (true) {
        std::vector<uint8_t> packet;
        int ret = socketCore.recv_data(packet);
        if (ret == 0) {

        } // TODO: Handle issues here

        // Decode message
        std::pair<uint8_t, std::vector<uint8_t>> data = protocolHandler->deconstruct_packet(packet);
        
        // Send to handler function
        if (incommingMessageHandler) {
            incommingMessageHandler->handle_message(data.first, data.second);
        }
    }
}

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

void DeviceClient::assign_receive_handler(std::shared_ptr<IEventHandler> handler) {
    incommingMessageHandler = std::move(handler);
}

void DeviceClient::start_listening() {
    if (!incommingMessageHandler) {
        std::cout << "NO INCOMMING HANDLER"; // TODO: Change to error
        return;
    }

    std::thread t(&DeviceClient::listen_loop, this);
    t.detach();
}

bool DeviceClient::send_packet(const std::vector<uint8_t>& packet) {
    int bytesSent = socketCore.send_data(packet);
    if (bytesSent != packet.size()) {
        // TODO: Handle not all bytes sent
        return false;
    }

    return true;
}

bool DeviceClient::assign_device_protocol(uint8_t deviceID) {
    switch(deviceID) {
        case (static_cast<uint8_t>(Protocol::DeviceIdentifier::FishTank)): {
            deviceType = DeviceType::FISH_TANK;
            protocolHandler = std::make_unique<FishTankProtocol>();

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

    // -- Send device identification request
    // Create DeviceAgnosticProtocol
    std::cout << "Creating DeviceAgnosticProtocol\n";
    DeviceAgnosticProtocol deviceAgnosticProtocol;

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
    uint8_t deviceID = 0xFF; // TODO: Changed to real
    success = assign_device_protocol(deviceID);
    if (!success) {
        std::cout << "No success";
    }
    
    return true;
}