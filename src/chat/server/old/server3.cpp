// This server uses a vector to keep track of all
// connected clients' socket file descriptors. When
// a client sends a message, the server reads it and
// broadcasts it to all other connected clients by
// iterating over the vector and sending the message
// using the send function. Each child process exits
// after handling a single message, and the parent
// process continues to listen for incoming connections.

#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const int MAX_CLIENTS = 10;

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno, clilen, n;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    std::cerr << "Error opening socket" << std::endl;
    return 1;
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = 1234;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Error on binding" << std::endl;
    return 1;
  }

  listen(sockfd, MAX_CLIENTS);

  std::vector<int> clients;
  while (true) {
    newsockfd =
        accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    if (newsockfd < 0) {
      std::cerr << "Error on accept" << std::endl;
      return 1;
    }

    int pid = fork();
    if (pid < 0) {
      std::cerr << "Error on fork" << std::endl;
      return 1;
    }

    if (pid == 0) {
      close(sockfd);
      while (true) {
        bzero(buffer, 256);
        n = recv(newsockfd, buffer, 255, 0);
        if (n < 0) {
          std::cerr << "Error reading from socket" << std::endl;
          break;
        }

        if (n == 0) {
          std::cout << "Client disconnected" << std::endl;
          break;
        }

        std::cout << "Received message: " << buffer << std::endl;

        for (int i = 0; i < clients.size(); i++) {
          if (clients[i] != newsockfd) {
            n = send(clients[i], buffer, n, 0);
            if (n < 0) {
              std::cerr << "Error writing to socket" << std::endl;
              break;
            }
          }
        }
      }

      close(newsockfd);
      return 0;
    } else {
      clients.push_back(newsockfd);
      close(newsockfd);
      waitpid(-1, NULL, WNOHANG);
    }
  }

  close(sockfd);
  return 0;
}
