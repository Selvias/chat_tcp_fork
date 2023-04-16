#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>

#include <set>

const int MAX_CONNECTIONS = 10;
const int MAX_MESSAGE_LEN = 256;

std::set<std::string> check = {};

struct Client {
  int socket;
  struct sockaddr_in address;
};

std::vector<Client> clients;

void add_client(Client client) { clients.push_back(client); }

void remove_client(int socket) {
  for (auto it = clients.begin(); it != clients.end(); it++) {
    if (it->socket == socket) {
      clients.erase(it);
      break;
    }
  }
}

void handle_client(int client_socket, sockaddr_in client_address) {
  char buffer[MAX_MESSAGE_LEN];
  std::string client_ip = inet_ntoa(client_address.sin_addr);
  std::cout << "New client connected: " << client_ip << std::endl;

  while (true) {
    memset(buffer, 0, sizeof(buffer)); // clear message buffer
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
      std::cout << "Client disconnected: " << client_ip << std::endl;
      close(client_socket);
      return;
    }

    std::string message = buffer;
    std::cout << "Received message from " << client_ip << ": " << message
              << std::endl;

    // TODO: Broadcast message to all other clients
    // for (int client_sock : connected_clients) {
    //   std::cout << "CLIENT IN LIST: " << client_sock << std::endl;
    send(client_socket, message.c_str(), message.length(), 0);
    // }
  }
}

void reaper(int signal) {
  int status;
  while (wait3(&status, WNOHANG, (rusage *)0) >= 0)
    ;
}

int main() {

  int server_fd, client_fd, addr_length, child_pid;
  sockaddr_in server_addr;
  char welcome_message[] = "Welcome to the chat room!\n";

  // Create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Server can't open socket for TCP\n");
    exit(EXIT_FAILURE);
  }

  // Zero initialization + Bind socket
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = 0;
  if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Server binding failure\n");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  addr_length = sizeof(server_addr);
  if (getsockname(server_fd, (sockaddr *)&server_addr,
                  (socklen_t *)&addr_length)) {
    perror("Calling getsockname() failure\n");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // second agrument - how many connections can be waiting for this
  // Listen for connections
  if (listen(server_fd, MAX_CONNECTIONS) < 0) {
    perror("Listening connections failure\n");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("SERVER: PORT - %d\n", ntohs(server_addr.sin_port));
  signal(SIGCHLD, reaper);
  // Handle incoming connections
  while (1) {
    sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_address,
                            &client_address_size)) < 0) {
      perror("Failed to accept connection\n");
      exit(EXIT_FAILURE);
    }

    child_pid = fork();
    if (child_pid < 0) {
      perror("Child-process creation failure\n");
      exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
      //   close(server_fd);
      //   int flag = 0;
      //   while (flag == 0) {
      //     flag = buff_work(client_fd);
      //   }
      //   close(client_fd);
      //   close(server_fd);
      //   handle_client(client_fd, client_address);
      Client client;
      client.socket = client_fd;
      client.address = client_address;
      add_client(client);

      // Send a welcome message to the new client
      send(client_fd, welcome_message, strlen(welcome_message), 0);

      // Broadcast the new client's name to all other clients
      char message[1024];
      sprintf(message, "%s has joined the chat room.\n",
              inet_ntoa(client_address.sin_addr));
      for (auto it = clients.begin(); it != clients.end(); it++) {
        if (it->socket != client_fd) {
          std::cout << message << std::endl;
          send(it->socket, message, strlen(message), 0);
        }
      }
      std::cout << "pesso\n";
      char buffer[1000] = {0};
      // Read messages from the client and broadcast them to all other clients
      while (true) {
        memset(buffer, 0, sizeof(buffer));
        // int bytes_read = read(client_fd, buffer, sizeof(buffer));
        int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        std::cout << buffer << std::endl;
        // for (auto it = clients.begin(); it != clients.end(); it++) {
        //   if (it->socket != client_fd) {
        //     std::cout << inet_ntoa(it->address.sin_addr) << std::endl;
        //   }
        // }
        check.insert(buffer);
        std::cout << "Buffer of the process:\n";
        for (const auto &str : check) {
          std::cout << str << std::endl;
        }
        // std::cout << "xui\n";
        // std::cout << "xui\n";
        // std::cout << "xui\n";
        // std::cout << "xui\n";
        if (bytes_read <= 0) {
          // The client has disconnected, remove them from the vector of
          // connected clients
          remove_client(client_fd);

          // Broadcast the client's name to all other clients
          char message[1024];
          sprintf(message, "%s has left the chat room.\n",
                  inet_ntoa(client_address.sin_addr));
          for (auto it = clients.begin(); it != clients.end(); it++) {
            if (it->socket != client_fd) {
              send(it->socket, message, strlen(message), 0);
            }
          }
          close(client_fd);
          break;
        }
        // Broadcast the message to all other clients
        // char message[1024];
        // sprintf(message, "[%s]: %s", inet_ntoa(client_address.sin_addr),
        //         buffer);
        for (auto it = clients.begin(); it != clients.end(); it++) {
          if (it->socket != client_fd) {
            send(it->socket, buffer, strlen(message), 0);
          }
        }
      }

      //   close(server_fd);
      exit(0);
    } else {
      printf("\t-----CHILD_PROCESS PID: %d -----\n\n", child_pid);
      close(client_fd);
    }
  }

  // Handle incoming connections
  // while (true) {
  //     struct sockaddr_in client_address;
  //     socklen_t client_address_size = sizeof(client_address);
  //     int client_socket = accept(server_socket, (struct sockaddr
  //     *)&client_address, &client_address_size);

  //     if (client_socket == -1) {
  //         std::cerr << "Failed to accept connection" << std::endl;
  //         continue;
  //     }

  //     pid_t pid = fork();
  //     if (pid == -1) {
  //         std::cerr << "Failed to fork process" << std::endl;
  //         close(client_socket);
  //         continue;
  //     } else if (pid == 0) {
  //         // Child process
  //         close(server_socket);
  //         handle_client(client_socket, client_address);
  //         return 0;
  //     } else {
  //         // Parent process
  //         close(client_socket);
  //     }
  // }

  close(server_fd);
  return 0;
}