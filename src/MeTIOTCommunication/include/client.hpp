#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <thread>

#include "core/socket_core.hpp"
#include "protocol/abstract_protocol.hpp"
#include "protocol/device_agnostic_protocol.hpp"
#include "protocol/fish_tank_protocol.hpp"
#include "interfaces/event_handler.hpp"

#define FATAL_WARNING_THRESHOLD 10

enum class DeviceType {
  UNKNOWN,
  FISH_TANK,
};

class DeviceClient {
  private:
    SocketCore socketCore;
    std::unique_ptr<AbstractProtocol> protocolHandler;
    std::shared_ptr<IEventHandler> callbackHandler;
    DeviceType deviceType;
    uint64_t uniqueDeviceID;

    void listen_loop();

    bool perform_device_initialization();

    bool assign_device_protocol(uint8_t deviceID);

  public:
    DeviceClient(const std::string& ip, int port) : socketCore(ip, port) {}

    /* @brief    Connects socket using IP and Port provided during initialization.
     *
     * @return   `True`  - On Success.
     * @return   `False` - On Failure.
     */
    bool connect();

    void assign_receive_handler(std::shared_ptr<IEventHandler> handler);

    void start_listening();

    /* @brief   Disconnects TCP socket.
     */
    void disconnect();

    /* @brief    Sends packet over TCP socket.
     *
     * @param    packet    The encoded packet to be sent.
     */
    void send_packet(const std::vector<uint8_t>& packet);

    uint64_t get_unique_id();

    DeviceType get_device_type() const { return deviceType; }
    AbstractProtocol* get_protocol_handler() const { return protocolHandler.get(); }
};

#endif
