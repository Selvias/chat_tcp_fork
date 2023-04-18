#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

#define PORT 8080
#define MAX_CLIENTS 10
#define SHM_SIZE (MAX_CLIENTS * sizeof(int))

struct message {
  long type;
  char text[256];
};

void reaper(int signal) {
  int status;
  while (wait3(&status, WNOHANG, (rusage *)0) >= 0)
    ;
}

int msgid;
key_t key;
int *client_fds;
message msg;

void *receive_messages(void *arg) {
  while (true) {
    if (msgrcv(msgid, &msg, sizeof(msg.text), 1, IPC_NOWAIT) == -1) {
      // No new messages in queue
      usleep(100000);
      continue;
    }

    // Broadcast message to all connected clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_fds[i] != 0) {
        if (send(client_fds[i], msg.text, strlen(msg.text), 0) == -1) {
          std::cerr << "Failed to send message to client\n";
        }
      }
    }
  }
  pthread_exit(NULL);
}
int server_fd, new_socket, shmid;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

void *handle_clients(void *arg) {
  signal(SIGCHLD, reaper);
  while (true) {
    // Accept new connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      cerr << "Accept error\n";
      exit(EXIT_FAILURE);
    }

    std::cout << "new_client: " << new_socket << std::endl;

    // Add new client to list of connected clients
    int i;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_fds[i] == 0) {
        client_fds[i] = new_socket;
        break;
      }
    }
    if (i == MAX_CLIENTS) {
      cerr << "Max clients reached\n";
      close(new_socket);
      continue;
    }

    // Fork new process to handle client messages
    pid_t pid = fork();
    if (pid == 0) {
      // Child process reads messages from client and broadcasts to all other
      // clients
      close(server_fd);
      while (true) {
        int nbytes = recv(new_socket, msg.text, sizeof(msg.text), 0);
        msg.text[nbytes] = '\0';
        std::cout << "recieved_msg: " << msg.text << std::endl;
        if (nbytes == 0) {
          // Client disconnected
          for (int j = 0; j < MAX_CLIENTS; j++) {
            if (client_fds[j] == new_socket) {
              client_fds[j] = 0;
              break;
            }
          }
          break;
        } else if (nbytes < 0) {
          // Error reading from client
          cerr << "Read error\n";
        }
        msg.type = 1;
        if (msgsnd(msgid, &msg, sizeof(msg.text), 0) == -1) {
          std::cerr << "Failed to send message to queue\n";
        }
        // exit(EXIT_FAILURE);
      }
      //   for (int j = 0; j < MAX_CLIENTS; j++) {
      //     if (client_fds[j] != 0 && client_fds[j] != new_socket) {
      //       if (send(client_fds[j], msg.text, strlen(msg.text), 0) < 0) {
      //         // Error sending message to client
      //         cerr << "Send error\n";
      //         exit(EXIT_FAILURE);
      //       }
      //     }
      //   }
    } else if (pid < 0) {
      // Fork error
      cerr << "Fork error\n";
      exit(EXIT_FAILURE);
    } else {
      // Parent process continues to accept new connections
      //   close(new_socket);
    }
  }
}

int main() {

  // Create shared memory segment for client file descriptors
  key = ftok("server", 'R');
  shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
  client_fds = (int *)shmat(shmid, NULL, 0);
  memset(client_fds, 0, SHM_SIZE);

  // Create message queue for broadcasting messages
  key = ftok("server", 'B');
  msgid = msgget(key, 0666 | IPC_CREAT);

  // Create server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    cerr << "Socket creation error\n";
    exit(EXIT_FAILURE);
  }

  // Set server socket options
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    cerr << "Setsockopt error\n";
    exit(EXIT_FAILURE);
  }

  // Bind server socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    cerr << "Bind error\n";
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_fd, MAX_CLIENTS) < 0) {
    cerr << "Listen error\n";
    exit(EXIT_FAILURE);
  }

  pthread_t receive_thread, handle_cl_thread;
  // Start the receive_messages thread
  if (pthread_create(&receive_thread, NULL, receive_messages, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&handle_cl_thread, NULL, handle_clients, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }
  //   Wait for the threads to finish
  pthread_join(receive_thread, NULL);
  pthread_join(handle_cl_thread, NULL);

  //   signal(SIGCHLD, reaper);
  //   while (true) {
  //     // Accept new connection
  //     if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
  //                              (socklen_t *)&addrlen)) < 0) {
  //       cerr << "Accept error\n";
  //       exit(EXIT_FAILURE);
  //     }

  //     std::cout << "new_client: " << new_socket << std::endl;

  //     // Add new client to list of connected clients
  //     int i;
  //     for (int i = 0; i < MAX_CLIENTS; i++) {
  //       if (client_fds[i] == 0) {
  //         client_fds[i] = new_socket;
  //         break;
  //       }
  //     }
  //     if (i == MAX_CLIENTS) {
  //       cerr << "Max clients reached\n";
  //       close(new_socket);
  //       continue;
  //     }

  //     // Fork new process to handle client messages
  //     pid_t pid = fork();
  //     if (pid == 0) {
  //       // Child process reads messages from client and broadcasts to all
  //       other
  //       // clients
  //       close(server_fd);
  //       while (true) {
  //         int nbytes = recv(new_socket, msg.text, sizeof(msg.text), 0);
  //         msg.text[nbytes] = '\0';
  //         std::cout << "recieved_msg: " << msg.text << std::endl;
  //         if (nbytes == 0) {
  //           // Client disconnected
  //           for (int j = 0; j < MAX_CLIENTS; j++) {
  //             if (client_fds[j] == new_socket) {
  //               client_fds[j] = 0;
  //               break;
  //             }
  //           }
  //           break;
  //         } else if (nbytes < 0) {
  //           // Error reading from client
  //           cerr << "Read error\n";
  //         }
  //         msg.type = 1;
  //         if (msgsnd(msgid, &msg, sizeof(msg.text), 0) == -1) {
  //           std::cerr << "Failed to send message to queue\n";
  //         }
  //         // exit(EXIT_FAILURE);
  //       }
  //       //   for (int j = 0; j < MAX_CLIENTS; j++) {
  //       //     if (client_fds[j] != 0 && client_fds[j] != new_socket) {
  //       //       if (send(client_fds[j], msg.text, strlen(msg.text), 0) < 0)
  //       {
  //       //         // Error sending message to client
  //       //         cerr << "Send error\n";
  //       //         exit(EXIT_FAILURE);
  //       //       }
  //       //     }
  //       //   }
  //     } else if (pid < 0) {
  //       // Fork error
  //       cerr << "Fork error\n";
  //       exit(EXIT_FAILURE);
  //     } else {
  //       // Parent process continues to accept new connections
  //       //   close(new_socket);
  //     }
  //   }

  //   while (true) {
  //     if (msgrcv(msgid, &msg, sizeof(msg.text), 1, IPC_NOWAIT) == -1) {
  //       // No new messages in queue
  //       usleep(100000);
  //       continue;
  //     }

  //     // Broadcast message to all connected clients
  //     for (int i = 0; i < MAX_CLIENTS; i++) {
  //       if (client_fds[i] != 0) {
  //         if (send(client_fds[i], msg.text, strlen(msg.text), 0) == -1) {
  //           std::cerr << "Failed to send message to client\n";
  //         }
  //       }
  //     }
  //   }

  // Detach and remove shared memory segment
  shmdt(client_fds);
  shmctl(shmid, IPC_RMID, NULL);

  // Remove message queue
  msgctl(msgid, IPC_RMID, NULL);
  pthread_exit(NULL);

  return 0;
}
