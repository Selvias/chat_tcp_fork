#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

// #define PORT 8082
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int clients[MAX_CLIENTS], num_clients = 0;

void broadcast(const char *message, int sender) {
  std::cout << message << "from: " << sender << std::endl;
  std::cout << "clients: ";
  for (int i = 0; i < num_clients; i++) {
    std::cout << clients[i] << " ";
    // if (clients[i] != sender) {
    send(clients[i], message, strlen(message), 0);
    // }
  }
  std::cout << std::endl;
}

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE];
  while (true) {
    memset(buffer, 0, BUFFER_SIZE);
    int n = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (n <= 0) {
      // client has disconnected
      for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client_socket) {
          clients[i] = clients[num_clients - 1];
          num_clients--;
          break;
        }
      }
      // close(client_socket);
      break;
    } else {
      buffer[n] = '\0';
      std::cout << "buffer: " << buffer << std::endl;
      // broadcast(buffer, client_socket);
    }
  }
}

void reaper(int signal) {
  int status;
  while (wait3(&status, WNOHANG, (rusage *)0) >= 0)
    ;
}

int main() {
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  int addr_length;
  sockaddr_in server_address;

  if (server_socket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
  }
  bzero((char *)&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = 0;

  if (bind(server_socket, (sockaddr *)&server_address,
           sizeof(server_address)) == -1) {
    std::cerr << "Failed to bind socket" << std::endl;
    close(server_socket);
    return 1;
  }
  addr_length = sizeof(server_address);
  if (getsockname(server_socket, (sockaddr *)&server_address,
                  (socklen_t *)&addr_length)) {
    perror("Server binding failure\n");
    exit(EXIT_FAILURE);
  }
  // std::cout << "Server port: " << ntohs(server_address.sin_port) <<
  // std::endl;

  if (listen(server_socket, MAX_CLIENTS) == -1) {
    std::cerr << "Failed to listen on socket" << std::endl;
    close(server_socket);
    return 1;
  }

  std::cout << "Listening on port " << ntohs(server_address.sin_port)
            << std::endl;
  signal(SIGCHLD, reaper);
  while (true) {
    sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int client_socket = accept(server_socket, (sockaddr *)&client_address,
                               &client_address_size);

    if (client_socket == -1) {
      std::cerr << "Failed to accept connection" << std::endl;
      continue;
    }

    if (num_clients == MAX_CLIENTS) {
      std::cout << "Maximum number of clients reached" << std::endl;
      close(client_socket);
      continue;
    }

    clients[num_clients++] = client_socket;

    pid_t pid = fork();
    if (pid == -1) {
      std::cerr << "Failed to fork" << std::endl;
      close(client_socket);
      num_clients--;
      continue;
    } else if (pid == 0) {
      // child process
      close(server_socket);
      std::cout << "New client connected: "
                << inet_ntoa(client_address.sin_addr) << std::endl;
      printf("SERVER: CLIENT_IP - %s, CLIENT_PORT - %d\n",
             inet_ntoa(client_address.sin_addr),
             ntohs(client_address.sin_port));
      handle_client(client_socket);
      std::cout << "Client disconnected: " << inet_ntoa(client_address.sin_addr)
                << std::endl;
      close(client_socket);
      exit(0);
    } else {
      // parent process
      printf("\t-----CHILD_PROCESS PID: %d -----\n\n", pid);
      close(client_socket);
    }
  }
  close(server_socket);
  return 0;
}
