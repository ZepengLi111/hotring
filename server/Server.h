//
// Created by lvzhi on 2024/7/11.
//

#ifndef HOTRING_SERVER_H
#define HOTRING_SERVER_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <unordered_map>
#include "HashTable.h"

#define BUCKET_SIZE 10

class Server {
private:
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;

public:
    HashTable<std::string, std::string> kv_store;
    Server(int port) : kv_store(BUCKET_SIZE) {
      // 创建 socket
      socket_desc = socket(AF_INET, SOCK_STREAM, 0);
      if (socket_desc == -1) {
        std::cerr << "Could not create socket" << std::endl;
        exit(1);
      }

      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      server.sin_port = htons(port);

      // 绑定
      if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        exit(1);
      }

      // 监听
      listen(socket_desc, 3);

      std::cout << "Waiting for incoming connections..." << std::endl;
      c = sizeof(struct sockaddr_in);
    }

    void start() {
      while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        std::cout << "Connection accepted" << std::endl;

        handle_client(client_sock);
        std::cout << "Hot Hits : " << kv_store.data->hotHit << std::endl;
        std::cout << "Adjust hotpot : " << kv_store.data->adjustHotpot << std::endl;
        std::cout << "All access count : " << kv_store.data->allAccessCount << std::endl;
        kv_store.print();
      }

      if (client_sock < 0) {
        std::cerr << "Accept failed" << std::endl;
        exit(1);
      }
    }

private:
    void handle_client(int sock) {
      char client_message[2000];
      int read_size;

      int count = 0;
      while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
        count++;
//        std::cout << "-------------------- " << count << std::endl;
        std::string message(client_message, read_size);
        std::istringstream iss(message);
        std::string key_put, value, key_read;
        iss >> key_put >> value >> key_read;
//        std::cout << "put key " << key_put << " value " << value << std::endl;
        std::string response;
        kv_store.put(key_put, value);
        auto res = kv_store.read(key_read);
        if (res.has_value()) {
          response = res.value();
        } else {
          response = "NULL";
        }
//        kv_store.print();
        send(sock, response.c_str(), response.length(), 0);
      }

      if (read_size == 0) {
        std::cout << "Client disconnected" << std::endl;
      } else if (read_size == -1) {
        std::cerr << "Receive failed" << std::endl;
      }

      close(sock);
    }
};


#endif //HOTRING_SERVER_H
