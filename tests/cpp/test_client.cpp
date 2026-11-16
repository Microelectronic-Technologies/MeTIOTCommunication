#include "../../src/MeTIOTCommunication/src/client.hpp"
#include "../../src/MeTIOTCommunication/src/core/exceptions.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/abstract_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/filter_guardian_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/protocol_constants.hpp"
#include "../../src/MeTIOTCommunication/src/protocol/tank_guardian_protocol.hpp"
#include "../../src/MeTIOTCommunication/src/utils/cobs.hpp"
#include "../../src/MeTIOTCommunication/src/utils/crc.hpp"

#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using Catch::Matchers::Equals;
using Catch::Matchers::WithinRel;

namespace {

// Helper to build a valid COBS + LE-CRC framed wire packet from raw header + payload
std::vector<uint8_t> make_wire_packet(uint8_t header, const std::vector<uint8_t>& payload = {}) {
    std::vector<uint8_t> raw = {header};
    raw.insert(raw.end(), payload.begin(), payload.end());

    uint16_t crc = calculate_crc(raw);
    std::vector<uint8_t> framed;
    framed.push_back(static_cast<uint8_t>(crc & 0xFF));
    framed.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
    framed.insert(framed.end(), raw.begin(), raw.end());

    return cobs_encode(framed);
}

// Helper to build a DeviceIdentifier response packet
std::vector<uint8_t> make_id_response(Protocol::DeviceIdentifier devId, uint64_t uniqueId) {
    std::vector<uint8_t> payload(9);
    payload[0] = static_cast<uint8_t>(devId);
    std::memcpy(&payload[1], &uniqueId, sizeof(uint64_t));

    return make_wire_packet(static_cast<uint8_t>(Protocol::NodeHeader::General::DeviceIdentifier), payload);
}

// Thread-safe mock IEventHandler with condition_variable synchronization
class MockEventHandler : public IEventHandler {
  public:
    struct MessageEvent {
        DeviceClient* client;
        int packet_id;
        std::vector<uint8_t> data;
    };

    struct ErrorEvent {
        DeviceClient* client;
        int code;
        std::string message;
    };

    void handle_message(DeviceClient* client, int packet_id, const std::vector<uint8_t>& data) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            messages_.push_back({client, packet_id, data});
        }
        cv_.notify_all();
    }

    void handle_warning(DeviceClient* client, int errorCode, const std::string& message) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            warnings_.push_back({client, errorCode, message});
        }
        cv_.notify_all();
    }

    void handle_fatal_error(DeviceClient* client, int errorCode, const std::string& message) override {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            fatal_errors_.push_back({client, errorCode, message});
        }
        cv_.notify_all();
    }

    bool wait_for_messages(size_t count, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(lock, timeout, [&]() { return messages_.size() >= count; });
    }

    bool wait_for_warnings(size_t count, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(lock, timeout, [&]() { return warnings_.size() >= count; });
    }

    bool wait_for_fatal(std::chrono::milliseconds timeout = std::chrono::milliseconds(1500)) {
        std::unique_lock<std::mutex> lock(mtx_);
        return cv_.wait_for(lock, timeout, [&]() { return !fatal_errors_.empty(); });
    }

    std::vector<MessageEvent> messages() {
        std::lock_guard<std::mutex> lock(mtx_);
        return messages_;
    }

    std::vector<ErrorEvent> warnings() {
        std::lock_guard<std::mutex> lock(mtx_);
        return warnings_;
    }

    std::vector<ErrorEvent> fatal_errors() {
        std::lock_guard<std::mutex> lock(mtx_);
        return fatal_errors_;
    }

  private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<MessageEvent> messages_;
    std::vector<ErrorEvent> warnings_;
    std::vector<ErrorEvent> fatal_errors_;
};

// Ephemeral TCP server simulating a MeT IoT hardware node
class MockDeviceServer {
  public:
    MockDeviceServer() {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        REQUIRE(listen_fd_ >= 0);

        int opt = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        REQUIRE(::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        REQUIRE(::listen(listen_fd_, 1) == 0);

        socklen_t len = sizeof(addr);
        REQUIRE(::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
        port_ = ntohs(addr.sin_port);
    }

    ~MockDeviceServer() {
        close_client();
        if (listen_fd_ >= 0)
            ::close(listen_fd_);
    }

    int port() const { return port_; }

    bool accept_client() {
        client_fd_ = ::accept(listen_fd_, nullptr, nullptr);
        return client_fd_ >= 0;
    }

    std::vector<uint8_t> recv_frame() {
        std::vector<uint8_t> frame;
        uint8_t byte = 0;
        while (true) {
            ssize_t n = ::recv(client_fd_, &byte, 1, 0);
            if (n <= 0)
                break;
            frame.push_back(byte);
            if (byte == 0x00)
                break; // COBS delimiter
        }
        return frame;
    }

    void send_bytes(const std::vector<uint8_t>& bytes) {
        ssize_t sent = ::send(client_fd_, bytes.data(), bytes.size(), 0);
        REQUIRE(sent == static_cast<ssize_t>(bytes.size()));
    }

    void close_client() {
        if (client_fd_ >= 0) {
            ::close(client_fd_);
            client_fd_ = -1;
        }
    }

  private:
    int listen_fd_ = -1;
    int client_fd_ = -1;
    int port_ = 0;
};

} // namespace

// ============================================================================
// 1. Pre-Connection Guards & LogicErrors
// ============================================================================

TEST_CASE("DeviceClient enforces initialization and handler preconditions", "[client][guards]") {
    DeviceClient client("127.0.0.1", 9999);

    SECTION("Accessing protocol handler or device type before connect throws LogicError") {
        CHECK_THROWS_AS(client.get_protocol_handler(), LogicError);
        CHECK_THROWS_AS(client.get_device_type(), LogicError);
    }

    SECTION("start_listening without an assigned handler throws LogicError") {
        CHECK_THROWS_AS(client.start_listening(), LogicError);
    }

    SECTION("connect throws SocketError when no server is listening") {
        int unused_port = 0;
        {
            MockDeviceServer temp;
            unused_port = temp.port();
        }
        DeviceClient badClient("127.0.0.1", unused_port);
        CHECK_THROWS_AS(badClient.connect(), SocketError);
    }
}

// ============================================================================
// 2. Handshake & Device Identification
// ============================================================================

TEST_CASE("DeviceClient performs handshake and selects correct device protocol", "[client][handshake]") {
    SECTION("Identifies TankGuardian device and stores unique ID") {
        MockDeviceServer server;
        const uint64_t expectedUID = 0x1122334455667788ULL;
        std::vector<uint8_t> receivedRequest;

        std::thread deviceThread([&]() {
            REQUIRE(server.accept_client());
            receivedRequest = server.recv_frame();
            server.send_bytes(make_id_response(Protocol::DeviceIdentifier::TankGuardian, expectedUID));
        });

        DeviceClient client("127.0.0.1", server.port());
        REQUIRE_NOTHROW(client.connect());
        deviceThread.join();

        // Verify client sent DeviceIdentificationRequest
        const std::vector<uint8_t> expectedReq =
            make_wire_packet(static_cast<uint8_t>(Protocol::ControllerHeader::General::DeviceIdentificationRequest));
        CHECK_THAT(receivedRequest, Equals(expectedReq));

        CHECK(client.get_unique_id() == expectedUID);
        CHECK(client.get_device_type() == DeviceType::TANK_GUARDIAN);
        CHECK(std::dynamic_pointer_cast<TankGuardianProtocol>(client.get_protocol_handler()) != nullptr);

        client.disconnect();
    }

    SECTION("Identifies FilterGuardian device") {
        MockDeviceServer server;
        const uint64_t expectedUID = 0xAABBCCDDEEFF0011ULL;

        std::thread deviceThread([&]() {
            REQUIRE(server.accept_client());
            (void)server.recv_frame();
            server.send_bytes(make_id_response(Protocol::DeviceIdentifier::FilterGuardian, expectedUID));
        });

        DeviceClient client("127.0.0.1", server.port());
        REQUIRE_NOTHROW(client.connect());
        deviceThread.join();

        CHECK(client.get_unique_id() == expectedUID);
        CHECK(client.get_device_type() == DeviceType::FILTER_GUARDIAN);
        CHECK(std::dynamic_pointer_cast<FilterGuardianProtocol>(client.get_protocol_handler()) != nullptr);

        client.disconnect();
    }

    SECTION("Skips unrelated telemetry packets arriving before DeviceIdentifier response") {
        MockDeviceServer server;
        const uint64_t expectedUID = 0x0000000000000042ULL;

        std::thread deviceThread([&]() {
            REQUIRE(server.accept_client());
            (void)server.recv_frame();

            // Send an unrelated data packet first, followed by the actual ID response
            server.send_bytes(make_wire_packet(0x20, {0x01, 0x02}));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            server.send_bytes(make_id_response(Protocol::DeviceIdentifier::TankGuardian, expectedUID));
        });

        DeviceClient client("127.0.0.1", server.port());
        REQUIRE_NOTHROW(client.connect());
        deviceThread.join();

        CHECK(client.get_unique_id() == expectedUID);
        CHECK(client.get_device_type() == DeviceType::TANK_GUARDIAN);

        client.disconnect();
    }

    SECTION("Throws ProtocolError if device never sends DeviceIdentifier within threshold") {
        MockDeviceServer server;

        std::thread deviceThread([&]() {
            REQUIRE(server.accept_client());
            (void)server.recv_frame();

            // Flood with non-ID packets until WRONG_PACKET_DURING_INIT_THRESHOLD is exceeded
            for (int i = 0; i < WRONG_PACKET_DURING_INIT_THRESHOLD + 2; ++i) {
                server.send_bytes(make_wire_packet(0x20, {0xFF}));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });

        DeviceClient client("127.0.0.1", server.port());
        CHECK_THROWS_AS(client.connect(), ProtocolError);

        deviceThread.join();
        client.disconnect();
    }
}

// ============================================================================
// 3. Asynchronous Listen Loop: Messages, Warnings & Fatal Escalation
// ============================================================================

TEST_CASE("DeviceClient listen_loop dispatches valid messages and rejects duplicate start", "[client][async]") {
    MockDeviceServer server;
    auto handler = std::make_shared<MockEventHandler>();

    std::thread deviceThread([&]() {
        REQUIRE(server.accept_client());
        (void)server.recv_frame(); // Consume ID request
        server.send_bytes(make_id_response(Protocol::DeviceIdentifier::TankGuardian, 12345));

        // Send a valid TankGuardian temperature reading (24.50 C -> 2450 -> LE {0x92, 0x09})
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::vector<uint8_t> sensorPayload = {static_cast<uint8_t>(Protocol::DataSubHeader::TankGuardian::Temperature),
                                              0x92, 0x09};
        server.send_bytes(make_wire_packet(0x10, sensorPayload));

        // Keep connection open until test verifies message, then close cleanly so listen_loop exits
        REQUIRE(handler->wait_for_messages(1));
        server.close_client();
    });

    DeviceClient client("127.0.0.1", server.port());
    client.assign_receive_handler(handler);
    REQUIRE_NOTHROW(client.connect());

    REQUIRE_NOTHROW(client.start_listening());
    // Starting a second time while active throws LogicError
    CHECK_THROWS_AS(client.start_listening(), LogicError);

    REQUIRE(handler->wait_for_messages(1));
    auto msgs = handler->messages();
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0].client == &client);
    CHECK(msgs[0].packet_id == 0x10);

    // Decode payload using client's assigned protocol handler
    auto decoded = client.get_protocol_handler()->interpret_data(msgs[0].data);
    CHECK_THAT(std::get<float>(decoded["Temperature_C"]), WithinRel(24.50f, 1e-4f));

    // Wait for listen_loop to exit on server EOF before destroying client
    REQUIRE(handler->wait_for_fatal());
    deviceThread.join();
    client.disconnect();
}

TEST_CASE("DeviceClient handles corrupt packets with warnings and escalates after threshold",
          "[client][async][warnings]") {
    MockDeviceServer server;
    auto handler = std::make_shared<MockEventHandler>();
    std::vector<uint8_t> rejectionFromClient;

    std::thread deviceThread([&]() {
        REQUIRE(server.accept_client());
        (void)server.recv_frame(); // Consume ID request
        server.send_bytes(make_id_response(Protocol::DeviceIdentifier::FilterGuardian, 999));

        // 1. Send a packet with a corrupted CRC -> triggers warning + MalformedPacketNotification
        std::vector<uint8_t> badPacket = make_wire_packet(0x10, {0x01, 0x02});
        badPacket[1] ^= 0xFF; // Corrupt CRC byte
        server.send_bytes(badPacket);

        // Read the rejection packet sent back by DeviceClient
        rejectionFromClient = server.recv_frame();

        // 2. Send remaining corrupt packets up to FATAL_WARNING_THRESHOLD to trigger fatal escalation
        for (int i = 1; i < FATAL_WARNING_THRESHOLD; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            server.send_bytes(badPacket);
        }
    });

    DeviceClient client("127.0.0.1", server.port());
    client.assign_receive_handler(handler);
    REQUIRE_NOTHROW(client.connect());
    REQUIRE_NOTHROW(client.start_listening());

    // Wait until fatal error stops the detached listen thread
    REQUIRE(handler->wait_for_fatal());
    deviceThread.join();

    // Client should have emitted (FATAL_WARNING_THRESHOLD - 1) warnings before the fatal error
    auto warnings = handler->warnings();
    CHECK(warnings.size() == static_cast<size_t>(FATAL_WARNING_THRESHOLD - 1));
    if (!warnings.empty()) {
        CHECK(warnings[0].code == static_cast<int>(AsyncErrorCode::LIBRARY_ERROR));
    }

    // Verify client sent MalformedPacketNotification back to the device
    const std::vector<uint8_t> expectedReject =
        make_wire_packet(static_cast<uint8_t>(Protocol::ControllerHeader::General::MalformedPacketNotification));
    CHECK_THAT(rejectionFromClient, Equals(expectedReject));

    // Verify fatal escalation details
    auto fatals = handler->fatal_errors();
    REQUIRE(fatals.size() == 1);
    CHECK(fatals[0].code == static_cast<int>(AsyncErrorCode::LIBRARY_ERROR));

    client.disconnect();
}

TEST_CASE("DeviceClient reports SOCKET_ERROR when peer disconnects during listen_loop", "[client][async][error]") {
    MockDeviceServer server;
    auto handler = std::make_shared<MockEventHandler>();

    std::thread deviceThread([&]() {
        REQUIRE(server.accept_client());
        (void)server.recv_frame();
        server.send_bytes(make_id_response(Protocol::DeviceIdentifier::TankGuardian, 777));

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        server.close_client(); // Abrupt peer disconnect
    });

    DeviceClient client("127.0.0.1", server.port());
    client.assign_receive_handler(handler);
    REQUIRE_NOTHROW(client.connect());
    REQUIRE_NOTHROW(client.start_listening());

    REQUIRE(handler->wait_for_fatal());
    deviceThread.join();

    auto fatals = handler->fatal_errors();
    REQUIRE(fatals.size() == 1);
    CHECK(fatals[0].code == static_cast<int>(AsyncErrorCode::SOCKET_ERROR));

    client.disconnect();
}
