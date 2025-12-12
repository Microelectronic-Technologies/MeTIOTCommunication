#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory> 

#include "core/socket_core.hpp"
#include "protocol/abstract_protocol.hpp"
#include "protocol/device_agnostic_protocol.hpp"
#include "protocol/fish_tank_protocol.hpp"

enum class DeviceType {
  UNKNOWN,
  FISH_TANK,
};

class DeviceClient {
  private:
    SocketCore socketCore;
    std::unique_ptr<AbstractProtocol> protocolHandler;
    DeviceType deviceType;

    bool perform_device_initialization();

    bool perform_ecdh(std::vector<uint8_t>& key);

    bool assign_device_protocol(uint8_t deviceID, std::vector<uint8_t>& key);

  public:
    DeviceClient(const std::string& ip, int port) : socketCore(ip, port) {}

    bool connect();

    void disconnect();

    bool send_packet(const std::vector<uint8_t>& packet);

    DeviceType get_device_type() const { return deviceType; }
    AbstractProtocol* get_protocol_handler() const { return protocolHandler.get(); }
};

#endif
