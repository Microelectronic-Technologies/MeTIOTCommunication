#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define MAX_RECV_SIZE 1024

class SocketCore {
  private:
    int clientSocket;
    std::string serverIP;
    int serverPORT;
    sockaddr_in serverAddress;

  public:
    SocketCore(const std::string& ip, int port);
    ~SocketCore();

    int connect_device();

    int disconnect();

    int send_data(const std::vector<uint8_t>& data);

    bool wait_for_data(int timeout_ms);

    std::vector<uint8_t> recv_data();
};
