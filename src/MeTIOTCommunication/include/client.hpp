#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory> 

#include "core/socket_core.hpp"
#include "protocol/abstract_protocol.hpp"

class DeviceClient {
  private:
    SocketCore socketCore;
    std::unique_ptr<AbstractProtocol> protocolHandler;

    bool perform_device_initialization();

    bool perform_ecdh();

  public:
    DeviceClient(const std::string& ip, int port) : socketCore(ip, port) {}

    bool connect();

    void disconnect();

    bool send_packet(const std::vector<uint8_t>& packet);

    AbstractProtocol* get_protocol_handler() const { return protocolHandler.get(); }
};

#endif
