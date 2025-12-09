#ifndef SOCKET_CORE_HPP
#define SOCKET_CORE_HPP

#include <string>
#include <vector>
#include <cstdint>

class SocketCore {
  private:
    int socket;

  public:
    SocketCore(const std::string& ip, int port);
    ~SocketCore();

    int connect();

    int disconnect();

    int send_data(const std::vector<uint8_t>& data);

    int recv_data();
};

#endif
