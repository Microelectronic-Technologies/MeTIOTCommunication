#include "../include/client.hpp"

void DeviceClient::listen_loop() {
    uint8_t warningCount = 0;

    // TODO: Remove debug
    uint8_t header = 0xFF;
    std::vector<uint8_t> data = {1,2,3,4};
    callbackHandler->handle_message(this, header, data);
    callbackHandler->handle_warning(this, "Test warning");
    callbackHandler->handle_fatal_error(this, "Test warning");
    return;

    while (true) {
        try {
            std::vector<uint8_t> packet;
            socketCore.recv_data(packet); // * This throws the fatal errors

            std::pair<uint8_t, std::vector<uint8_t>> data = protocolHandler->deconstruct_packet(packet); // * This throws the warning errors

            callbackHandler->handle_message(this, data.first, data.second);
            warningCount = 0; // Reset warning count after a single successful recv
        } catch (const SocketError& e) { // Catch specific fatal first
            // Fatal error & kill thread
            callbackHandler->handle_fatal_error(this, e.what());
            return;
        } catch (const LibraryError& e) { // Catch rest of error types (non fatal) including Protocol & Encoding
            warningCount += 1;

            if (warningCount >= FATAL_WARNING_THRESHOLD) {
                callbackHandler->handle_fatal_error(this, e.what());
                return;
            }

            // Notify python dev via callback
            callbackHandler->handle_warning(this, e.what());

            // Send a packet reject to client
            std::vector<uint8_t> packet = protocolHandler->create_rejection_packet();

            try {
                send_packet(packet);
            } catch (const SocketError& e) {
                // Fatal Error. Stop thread
                callbackHandler->handle_fatal_error(this, e.what());
                return;
            }
        } catch (const std::exception& e) {
            callbackHandler->handle_fatal_error(this, "Unexpected System Error: " + std::string(e.what()));
            return;
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
    callbackHandler = std::move(handler);
}

void DeviceClient::start_listening() {
    if (!callbackHandler) {
        std::cout << "NO INCOMMING HANDLER"; // TODO: Change to error
        return;
    }

    std::thread t(&DeviceClient::listen_loop, this);
    t.detach();
}

void DeviceClient::send_packet(const std::vector<uint8_t>& packet) {
    int bytesSent = socketCore.send_data(packet);
    if (bytesSent == -1) {
        throw SocketError("Socket: Error occured when sending packet.");
    } else if (bytesSent != packet.size()) {
        throw SocketError("Socket: Failed to send all bytes when sending packet.");
    }
}

uint64_t DeviceClient::get_unique_id() {
    return uniqueDeviceID;
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
    

    // -- Save uniqueID
    uniqueDeviceID = 0x1234123412341234; // TODO: Change to real

    // -- Assign specific protocol to protocolHandler (possible push to helper function)
    uint8_t deviceID = 0xFF; // TODO: Changed to real
    success = assign_device_protocol(deviceID);
    if (!success) {
        std::cout << "No success";
    }
    
    return true;
}