//
// Created by lvzhi on 2024/7/11.
//

#ifndef HOTRING_CLIENT_CLIENT_H
#define HOTRING_CLIENT_CLIENT_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class Client {
private:
  int sock;
  struct sockaddr_in server;
public:
  Client(const char* server_address, int port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      std::cerr << "Could not create socket" << std::endl;
      exit(1);
    }
    server.sin_addr.s_addr = inet_addr(server_address);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
      std::cerr << "Connect failed" << std::endl;
      exit(1);
    }
    std::cout << "Connected to server" << std::endl;
  }

  void send_message(const std::string& message) {
    if (send(sock, message.c_str(), message.length(), 0) < 0) {
      std::cerr << "Send failed" << std::endl;
      exit(1);
    }
  }

  std::string receive_message() {
    char buffer[1024];
    int read_size = recv(sock, buffer, sizeof(buffer), 0);
    if (read_size <= 0) {
      std::cerr << "Receive failed" << std::endl;
      exit(1);
    }
    return std::string(buffer, read_size);
  }

  ~Client() {
    close(sock);
  }
};


#endif //HOTRING_CLIENT_CLIENT_H
