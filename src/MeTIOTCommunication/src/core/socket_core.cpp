#include "../include/core/socket_core.hpp"

SocketCore::SocketCore(const std::string& ip, int port) {
    serverIP = ip;
    serverPORT = port;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        // TODO: Handle error
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPORT);

    if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr)) <= 0) {
        // TODO: Handle error. Invalid address or address not supported
    }
}

SocketCore::~SocketCore() {

}

int SocketCore::connectDevice() {
    return connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
}

int SocketCore::disconnect() {
    return close(clientSocket);
}

int SocketCore::send_data(const std::vector<uint8_t>& data) {
    return send(clientSocket, data.data(), data.size(), 0);
}

int SocketCore::recv_data(std::vector<uint8_t>& buffer) {
    buffer.resize(MAX_RECV_SIZE);

    ssize_t bytes_received = recv(clientSocket, buffer.data(), MAX_RECV_SIZE, 0);

    if (bytes_received > 0) {
        buffer.resize(bytes_received);
    } else if (bytes_received == 0) {
        // TODO: Connection closed by the peer
        return 0;
    } else {
        // TODO: Error occurred
        return -1;
    }

    return bytes_received;
}