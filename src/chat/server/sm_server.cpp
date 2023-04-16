// #include <arpa/inet.h>
// #include <cstdio>
// #include <cstdlib>
// #include <cstring>
// #include <iostream>
// #include <netinet/in.h>
// #include <string>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>

// const int MAX_CLIENTS = 10;

// struct client_info {
//   int sockfd;
//   sockaddr_in addr;
// };

// struct shared_data {
//   int num_clients;
//   client_info clients[MAX_CLIENTS];
// };

// void error(const char *msg) {
//   perror(msg);
//   exit(1);
// }

// int main(int argc, char *argv[]) {
//   int sockfd, portno, shmid;
//   socklen_t clilen;
//   struct sockaddr_in serv_addr, cli_addr;
//   char buffer[256];

//   if (argc < 2) {
//     std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
//     exit(1);
//   }

//   sockfd = socket(AF_INET, SOCK_STREAM, 0);
//   if (sockfd < 0) {
//     error("Error opening socket");
//   }

//   memset((char *)&serv_addr, 0, sizeof(serv_addr));
//   portno = std::atoi(argv[1]);
//   serv_addr.sin_family = AF_INET;
//   serv_addr.sin_addr.s_addr = INADDR_ANY;
//   serv_addr.sin_port = htons(portno);

//   if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//     error("Error on binding");
//   }

//   listen(sockfd, 5);

//   // create shared memory segment for storing client data
//   shmid = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | 0666);
//   if (shmid < 0) {
//     error("Error creating shared memory segment");
//   }

//   // attach shared memory segment to this process
//   shared_data *data = (shared_data *)shmat(shmid, NULL, 0);
//   if (data == (shared_data *)(-1)) {
//     error("Error attaching shared memory segment");
//   }

//   data->num_clients = 0;

//   clilen = sizeof(cli_addr);
//   while (true) {
//     int client_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
//     if (client_fd < 0) {
//       error("Error on accept");
//     }

//     pid_t pid = fork();
//     if (pid < 0) {
//       error("Error on fork");
//     }

//     if (pid == 0) {
//       // child process
//       close(sockfd);

//       // add client info to shared memory
//       client_info info;
//       info.sockfd = client_fd;
//       info.addr = cli_addr;
//       data->clients[data->num_clients] = info;
//       ++data->num_clients;

//       // notify all clients of new connection
//       for (int i = 0; i < data->num_clients; ++i) {
//         std::string msg =
//             "New client connected from " +
//             std::string(inet_ntoa(cli_addr.sin_addr)) + ":" +
//             std::to_string(ntohs(cli_addr.sin_port)) + ". There are now " +
//             std::to_string(data->num_clients) + " clients connected.";
//         send(data->clients[i].sockfd, msg.c_str(), msg.length(), 0);
//       }

//       while (true) {
//         int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
//         if (n < 0) {
//           error("Error on recv");
//         }

//         buffer[n] = '\0';

//         // broadcast message to all clients

//         for (int i = 0; i < data->num_clients; ++i) {
//           if (data->clients[i].sockfd != client_fd) {
//             std::string msg =
//                 "Client " + std::string(inet_ntoa(cli_addr.sin_addr)) + ":" +
//                 std::to_string(ntohs(cli_addr.sin_port)) + " says: " +
//                 buffer;
//             send(data->clients[i].sockfd, msg.c_str(), msg.length(), 0);
//           }
//         }

//         if (std::strncmp(buffer, "exit", 4) == 0) {
//           break;
//         }
//       }

//       // remove client info from shared memory
//       for (int i = 0; i < data->num_clients; ++i) {
//         if (data->clients[i].sockfd == client_fd) {
//           --data->num_clients;
//           for (int j = i; j < data->num_clients; ++j) {
//             data->clients[j] = data->clients[j + 1];
//           }
//           break;
//         }
//       }

//       // notify all clients of disconnected client
//       for (int i = 0; i < data->num_clients; ++i) {
//         std::string msg =
//             "Client " + std::string(inet_ntoa(cli_addr.sin_addr)) + ":" +
//             std::to_string(ntohs(cli_addr.sin_port)) +
//             " has disconnected. There are now " +
//             std::to_string(data->num_clients) + " clients connected.";
//         send(data->clients[i].sockfd, msg.c_str(), msg.length(), 0);
//       }

//       close(client_fd);

//       exit(0);
//     } else {
//       // parent process
//       close(client_fd);
//     }
//   }

//   // detach shared memory segment from this process
//   if (shmdt(data) < 0) {
//     error("Error detaching shared memory segment");
//   }

//   // destroy shared memory segment
//   if (shmctl(shmid, IPC_RMID, NULL) < 0) {
//     error("Error destroying shared memory segment");
//   }

//   return 0;
// }

// version 2
//  #include <arpa/inet.h>
//  #include <iostream>
//  #include <stdlib.h>
//  #include <string.h>
//  #include <sys/ipc.h>
//  #include <sys/shm.h>
//  #include <sys/socket.h>
//  #include <sys/types.h>
//  #include <sys/wait.h>
//  #include <unistd.h>
//  #include <vector>

// #define PORT 8080
// #define SHM_SIZE 1024

// int main() {
//   int server_fd, new_socket, valread;
//   struct sockaddr_in address;
//   int opt = 1;
//   int addrlen = sizeof(address);
//   char buffer[1024] = {0};
//   pid_t child_pid;
//   key_t key = 1234;
//   int shmid;
//   char *shmaddr;
//   std::vector<int> client_sockets;

//   // create shared memory segment
//   if ((shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666)) < 0) {
//     perror("shmget");
//     exit(1);
//   }

//   // attach shared memory segment
//   if ((shmaddr = (char *)shmat(shmid, NULL, 0)) == (char *)-1) {
//     perror("shmat");
//     exit(1);
//   }

//   // create server socket
//   if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//     perror("socket failed");
//     exit(EXIT_FAILURE);
//   }

//   // set socket options
//   if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
//                  sizeof(opt))) {
//     perror("setsockopt");
//     exit(EXIT_FAILURE);
//   }

//   // bind server socket to port
//   address.sin_family = AF_INET;
//   address.sin_addr.s_addr = INADDR_ANY;
//   address.sin_port = htons(PORT);

//   if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//     perror("bind failed");
//     exit(EXIT_FAILURE);
//   }

//   // listen for incoming connections
//   if (listen(server_fd, 3) < 0) {
//     perror("listen");
//     exit(EXIT_FAILURE);
//   }

//   while (1) {
//     // accept incoming connection
//     if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
//                              (socklen_t *)&addrlen)) < 0) {
//       perror("accept");
//       exit(EXIT_FAILURE);
//     }

//     // add client socket to vector
//     client_sockets.push_back(new_socket);

//     // fork child process to handle client request
//     if ((child_pid = fork()) == 0) {
//       close(server_fd);

//       while (1) {
//         // read message from shared memory
//         std::string message(shmaddr);
//         memset(shmaddr, 0, SHM_SIZE);

//         // send message to client send(new_socket, message.c_str(),
//         // message.length(), 0);

//         // receive message from client
//         valread = recv(new_socket, buffer, 1024, 0);

//         // write message to shared memory
//         strncpy(shmaddr, buffer, SHM_SIZE);
//         std::cout << shmaddr << std::endl;

//         // broadcast message to other clients
//         std::string broadcast_message(shmaddr);
//         memset(shmaddr, 0, SHM_SIZE);

//         for (auto socket : client_sockets) {
//           //   if (socket != new_socket) {
//           send(socket, broadcast_message.c_str(), broadcast_message.length(),
//                0);
//           //   }
//         }

//         // clear buffer
//         memset(buffer, 0, 1024);
//       }

//       exit(0);
//     } else {
//       close(new_socket);
//     }
//   }

//   // detach shared memory segment
//   if (shmdt(shmaddr) == -1) {
//     perror("shmdt");
//     exit(1);
//   }
//   // delete shared memory segment
//   if (shmctl(shmid, IPC_RMID, NULL) == -1) {
//     perror("shmctl");
//     exit(1);
//   }

//   return 0;
// }

// version 3
#include <arpa/inet.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define PORT 8080
#define SHM_SIZE 1024
#define SHM_KEY 1234
#define MSG_KEY 5678

int main() {
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char *message;
  pid_t child_pid;
  int shmid, msgid;
  char *shmaddr, *msgaddr;
  std::vector<int> *client_sockets;

  // create shared memory segment for client sockets vector
  if ((shmid = shmget(SHM_KEY, sizeof(std::vector<int>), IPC_CREAT | 0666)) <
      0) {
    perror("shmget");
    exit(1);
  }

  // attach shared memory segment
  if ((shmaddr = (char *)shmat(shmid, NULL, 0)) == (char *)-1) {
    perror("shmat");
    exit(1);
  }

  // initialize client sockets vector in shared memory segment
  client_sockets = new (shmaddr) std::vector<int>();

  // create shared memory segment for message buffer
  if ((msgid = shmget(MSG_KEY, SHM_SIZE, IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  // attach shared memory segment
  if ((msgaddr = (char *)shmat(msgid, NULL, 0)) == (char *)-1) {
    perror("shmat");
    exit(1);
  }

  // set initial message to empty string
  message = new (msgaddr) char[SHM_SIZE];
  message[0] = '\0';

  // create server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // set socket options
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // bind server socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  // listen for incoming connections
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    send(new_socket, "hello", ((std::string) "hello").length(), 0);

    // add client socket to vector
    client_sockets->push_back(new_socket);

    // fork child process to handle client request
    if ((child_pid = fork()) == 0) {
      close(server_fd);

      while (1) {
        // receive message from client
        valread = recv(new_socket, msgaddr, 1024, 0);
        message[valread] = '\0';
        // broadcast message to all other clients
        for (auto it = client_sockets->begin(); it != client_sockets->end();
             ++it) {
          //   if (*it != new_socket) {
          send(*it, msgaddr, strlen(msgaddr), 0);
          //   }
        }
      }

      // detach from shared memory segments
      shmdt(shmaddr);
      shmdt(msgaddr);

      return 0;
    }
  }

  // detach from shared memory segments
  shmdt(shmaddr);
  shmdt(msgaddr);

  // delete shared memory segments
  shmctl(shmid, IPC_RMID, NULL);
  shmctl(msgid, IPC_RMID, NULL);

  return 0;
}
