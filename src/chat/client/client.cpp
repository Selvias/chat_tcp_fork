// #include <arpa/inet.h>
// #include <cstring>
// #include <iostream>
// #include <sys/socket.h>
// #include <unistd.h>

// int main() {
//   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//   if (sockfd < 0) {
//     std::cerr << "Error creating socket\n";
//     return 1;
//   }

//   struct sockaddr_in serv_addr;
//   std::memset(&serv_addr, 0, sizeof(serv_addr));

//   serv_addr.sin_family = AF_INET;
//   serv_addr.sin_port = htons(8080);

//   if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
//     std::cerr << "Invalid address/ Address not supported\n";
//     return 1;
//   }

//   if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
//   {
//     std::cerr << "Connection Failed\n";
//     return 1;
//   }

//   std::cout << "Connected to server\n";

//   char buffer[1024] = {0};
//   while (true) {
//     std::cout << "Enter message: ";
//     std::cin.getline(buffer, 1024);

//     send(sockfd, buffer, std::strlen(buffer), 0);

//     if (std::strncmp(buffer, "exit", 4) == 0) {
//       break;
//     }

//     std::memset(buffer, 0, sizeof(buffer));
//     int n = recv(sockfd, buffer, sizeof(buffer), 0);
//     if (n < 0) {
//       std::cerr << "Error receiving data from server\n";
//       break;
//     } else if (n == 0) {
//       std::cout << "Server disconnected\n";
//       break;
//     } else {
//       std::cout << "Server says: " << buffer << "\n";
//     }
//   }

//   close(sockfd);

//   return 0;
// }

// version 2
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <ip_address> <port_number>\n";
    return 1;
  }

  // create socket
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    std::cerr << "Failed to create socket\n";
    return 1;
  }

  // specify server address and port
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(argv[1]);
  server_address.sin_port = htons(std::stoi(argv[2]));

  // connect to server
  if (connect(client_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    std::cerr << "Failed to connect to server\n";
    return 1;
  }

  char buffer[BUFFER_SIZE] = {0};

  // read server response
  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
  if (bytes_received < 0) {
    std::cerr << "Failed to receive message from server\n";
    return 1;
  }
  std::cout << "Server response: " << buffer << std::endl;

  while (true) {
    // prompt user for message
    std::cout << "Enter message (or type 'exit' to quit): ";
    std::string message;
    std::getline(std::cin, message);

    // send message to server
    int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
    if (bytes_sent < 0) {
      std::cerr << "Failed to send message to server\n";
      break;
    }

    // check if user wants to quit
    if (message == "exit") {
      break;
    }

    // read server response
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
      std::cerr << "Failed to receive message from server\n";
      break;
    }
    std::cout << "Server response: " << buffer << std::endl;
  }

  // send disconnect message to server
  std::string disconnect_message = "exit";
  int bytes_sent = send(client_socket, disconnect_message.c_str(),
                        disconnect_message.length(), 0);
  if (bytes_sent < 0) {
    std::cerr << "Failed to send disconnect message to server\n";
  }

  // close socket
  close(client_socket);

  return 0;
}
