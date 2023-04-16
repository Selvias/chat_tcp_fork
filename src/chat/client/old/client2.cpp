
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"
#define PORT 42269
#define BUFFER_SIZE 1024

int main() {
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
  }

  std::cout << "socket: " << client_socket << std::endl;

  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
  server_address.sin_port = htons(PORT);

  if (connect(client_socket, (sockaddr *)&server_address,
              sizeof(server_address)) == -1) {
    std::cerr << "Failed to connect to server" << std::endl;
    close(client_socket);
    return 1;
  }

  std::cout << "Connected to server" << std::endl;

  char buffer[BUFFER_SIZE];
  char recv_buffer[BUFFER_SIZE];
  while (true) {
    memset(buffer, 0, BUFFER_SIZE);
    std::cout << "> ";
    std::cin.getline(buffer, BUFFER_SIZE);
    send(client_socket, buffer, strlen(buffer), 0);
    recv(client_socket, recv_buffer, BUFFER_SIZE, 0);
  }

  close(client_socket);
  return 0;
}
