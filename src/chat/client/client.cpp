#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace client {
const int MAX_MESSAGE_SIZE = 1024;
const int MAX_USERNAME_LEN = 20;

int new_socket;
char message[MAX_MESSAGE_SIZE];
std::string full_message;
int is_it_me = 0;

void *receive_messages(void *arg) {
  int bytes_read;
  while (true) {
    // Receive messages from the server
    bytes_read = read(new_socket, message, MAX_MESSAGE_SIZE);
    if (bytes_read == 0) {
      std::cout << "Server closed the connection" << std::endl;
      break;
    } else if (bytes_read < 0) {
      perror("Error reading from socket");
      break;
    } else {
      if (is_it_me) {
        is_it_me = 0;
        continue;
      }
      message[bytes_read] = '\0';
      std::cout << message << std::endl;
    }
  }
  pthread_exit(NULL);
}

void *send_messages(void *arg) {
  char username[MAX_USERNAME_LEN];
  memset(username, 0, MAX_USERNAME_LEN);
  std::cout << "Enter your username: ";
  std::cin >> username;
  std::cin.get();
  send(new_socket, username, MAX_USERNAME_LEN, 0);
  while (true) {
    // Get input from the user
    if (fgets(message, MAX_MESSAGE_SIZE, stdin) != 0) {
      // Send the message to the server
      full_message += username;
      full_message += ": ";
      full_message += message;
      if ((send(new_socket, full_message.c_str(), full_message.length(), 0)) <
          0) {
        perror("Error sending message");
        break;
      }
      is_it_me = 1;
      full_message = "";
    }
  }
  pthread_exit(NULL);
}
} // namespace client

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <server IP address> <server port>"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  char *ip_address_str = argv[1];
  int port = atoi(argv[2]);
  // convert IP address string to binary form
  in_addr ip_address;
  if ((inet_pton(AF_INET, ip_address_str, &ip_address)) == 0) {
    std::cerr << "Error: invalid IP address " << ip_address_str << std::endl;
    exit(1);
  }
  // int new_socket = socket(AF_INET, SOCK_STREAM, 0);
  client::new_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client::new_socket < 0) {
    std::cerr << "Error creating socket" << std::endl;
    return 1;
  }

  // Configure the server address
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr = ip_address;
  server_address.sin_port = htons(port);

  if ((connect(client::new_socket, (struct sockaddr *)&server_address,
               sizeof(server_address))) == -1) {
    perror("connect");
    exit(1);
  }

  pthread_t receive_thread, send_thread;
  // Start the receive_messages thread
  if (pthread_create(&receive_thread, NULL, client::receive_messages, NULL) <
      0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }

  // Start the send_messages thread
  if (pthread_create(&send_thread, NULL, client::send_messages, NULL) < 0) {
    perror("Error creating send thread");
    exit(EXIT_FAILURE);
  }

  // Wait for the threads to finish
  pthread_join(receive_thread, NULL);
  pthread_join(send_thread, NULL);

  // Close the client socket and exit
  close(client::new_socket);
  pthread_exit(NULL);
}