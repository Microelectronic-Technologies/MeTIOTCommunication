#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "socket_core.hpp"

class DeviceClient {
  private:
    SocketCore socketCore;

    bool perform_device_initialization();

    bool perform_ecdh();

  public:
    DeviceClient(const std::string& ip, int port) : socketCore(ip, port) {}
    ~DeviceClient();

    bool connect();

    void disconnect();

    bool send_command(uint8_t command);

    bool send_command(uint8_t command, std::vector<uint8_t>& payload);
};

#endif
