#include "../../src/MeTIOTCommunication/src/core/exceptions.hpp"
#include "../../src/MeTIOTCommunication/src/core/socket_core.hpp"

#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <chrono>
#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

// Minimal RAII loopback TCP server bound to 127.0.0.1 on an OS-assigned port
class LoopbackServer {
  public:
    LoopbackServer() {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        REQUIRE(listen_fd_ >= 0);

        int opt = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
        addr.sin_port = 0;                             // OS picks an unused ephemeral port

        REQUIRE(::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
        REQUIRE(::listen(listen_fd_, 1) == 0);

        socklen_t len = sizeof(addr);
        REQUIRE(::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
        port_ = ntohs(addr.sin_port);
    }

    ~LoopbackServer() {
        close_client();
        if (listen_fd_ >= 0) {
            ::close(listen_fd_);
        }
    }

    int port() const { return port_; }

    int accept_client() {
        client_fd_ = ::accept(listen_fd_, nullptr, nullptr);
        return client_fd_;
    }

    void send_bytes(const std::vector<uint8_t>& bytes) {
        REQUIRE(client_fd_ >= 0);
        ssize_t sent = ::send(client_fd_, bytes.data(), bytes.size(), 0);
        REQUIRE(sent == static_cast<ssize_t>(bytes.size()));
    }

    std::vector<uint8_t> recv_bytes(size_t expected_len) {
        REQUIRE(client_fd_ >= 0);
        std::vector<uint8_t> buf(expected_len);
        size_t total = 0;
        while (total < expected_len) {
            ssize_t n = ::recv(client_fd_, buf.data() + total, expected_len - total, 0);
            if (n <= 0)
                break;
            total += static_cast<size_t>(n);
        }
        buf.resize(total);
        return buf;
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

TEST_CASE("SocketCore constructor validates IP addresses", "[socket][constructor]") {
    SECTION("Valid IPv4 address constructs cleanly") {
        REQUIRE_NOTHROW([&]() {
            SocketCore client("127.0.0.1", 8080);
            client.disconnect();
        }());
    }

    SECTION("Malformed IP string throws SocketError") {
        CHECK_THROWS_AS(SocketCore("not.an.ip.address", 8080), SocketError);
        CHECK_THROWS_AS(SocketCore("999.999.999.999", 8080), SocketError);
        CHECK_THROWS_AS(SocketCore("", 8080), SocketError);
    }
}

TEST_CASE("SocketCore connects and disconnects cleanly", "[socket][lifecycle]") {
    LoopbackServer server;
    std::thread accept_thread([&]() { server.accept_client(); });

    SocketCore client("127.0.0.1", server.port());

    CHECK(client.connect_device() == 0);
    CHECK(client.disconnect() == 0);

    // Calling disconnect a second time returns -1 (EBADF) without throwing
    CHECK(client.disconnect() == -1);

    accept_thread.join();
}

TEST_CASE("SocketCore handles connection refused", "[socket][error]") {
    int unused_port = 0;
    {
        LoopbackServer temp;
        unused_port = temp.port();
    }

    SocketCore client("127.0.0.1", unused_port);
    CHECK(client.connect_device() == -1);

    // Prevent SIGPIPE from killing the test runner if send() is called on an unconnected fd
    std::signal(SIGPIPE, SIG_IGN);
    CHECK(client.send_data({0x01, 0x02}) == -1);

    client.disconnect();
}

TEST_CASE("SocketCore send_data transmits exact binary payload including null bytes", "[socket][io]") {
    LoopbackServer server;
    const std::vector<uint8_t> payload = {0xAA, 0x00, 0xFF, 0x10, 0x00, 0x55};
    std::vector<uint8_t> server_received;

    std::thread server_thread([&]() {
        if (server.accept_client() >= 0) {
            server_received = server.recv_bytes(payload.size());
        }
    });

    SocketCore client("127.0.0.1", server.port());
    REQUIRE(client.connect_device() == 0);

    int bytes_sent = client.send_data(payload);
    CHECK(bytes_sent == static_cast<int>(payload.size()));

    server_thread.join();
    CHECK_THAT(server_received, Catch::Matchers::Equals(payload));

    client.disconnect();
}

TEST_CASE("SocketCore wait_for_data and recv_data handle timeouts and incoming frames", "[socket][io]") {
    LoopbackServer server;
    const std::vector<uint8_t> reply = {0xDE, 0xAD, 0xBE, 0xEF};

    std::thread server_thread([&]() {
        if (server.accept_client() >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            server.send_bytes(reply);
        }
    });

    SocketCore client("127.0.0.1", server.port());
    REQUIRE(client.connect_device() == 0);

    // 1. Times out before server sends data
    CHECK_FALSE(client.wait_for_data(20));

    // 2. Returns true once data arrives
    REQUIRE(client.wait_for_data(500));

    // 3. Reads exact bytes sent by server
    std::vector<uint8_t> received = client.recv_data();
    CHECK_THAT(received, Catch::Matchers::Equals(reply));

    server_thread.join();
    client.disconnect();
}

TEST_CASE("SocketCore throws SocketError when remote peer disconnects", "[socket][eof]") {
    LoopbackServer server;

    std::thread server_thread([&]() {
        if (server.accept_client() >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            server.close_client();
        }
    });

    SocketCore client("127.0.0.1", server.port());
    REQUIRE(client.connect_device() == 0);

    // Either wait_for_data() catches POLLHUP or recv_data() sees 0 bytes and throws SocketError
    CHECK_THROWS_AS(
        [&]() {
            if (client.wait_for_data(500)) {
                (void)client.recv_data();
            }
        }(),
        SocketError);

    server_thread.join();
    client.disconnect();
}

TEST_CASE("SocketCore throws SocketError when polling or reading a closed socket", "[socket][error]") {
    LoopbackServer server;
    std::thread accept_thread([&]() { server.accept_client(); });

    SocketCore client("127.0.0.1", server.port());
    REQUIRE(client.connect_device() == 0);
    REQUIRE(client.disconnect() == 0);

    // POLLNVAL on closed fd
    CHECK_THROWS_AS(client.wait_for_data(10), SocketError);
    // EBADF on closed fd
    CHECK_THROWS_AS(client.recv_data(), SocketError);

    accept_thread.join();
}
