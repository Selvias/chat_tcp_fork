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
#include <vector>

#define PORT 8080
#define MAX_CLIENTS 10
#define SHM_SIZE (MAX_CLIENTS * sizeof(int))
#define MAX_NAME_LENGTH 20

struct message {
  long type;
  char text[1024];
};

struct client_data {
  char name[20];
  int is_just_connected = 0;
  int is_just_leave = 0;
};

// broadcast data
int msgid;
key_t key;
message msg;

// shared data (logic)
int *client_fds;
int *client_lost_cnnct;
client_data *client_dt;

int server_fd, new_socket, shmid, shmid_lost, shmid_clients;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

std::vector<int> shmid_list;

void reaper(int signal) {
  int status;
  while (wait3(&status, WNOHANG, (rusage *)0) >= 0)
    ;
}

void cleanup(int signal) {
  // detach and remove shared memory segments
  // shmdt(clients);
  shmdt(client_fds);
  // shmdt(client_lost_cnnct);
  for (const int &shmid : shmid_list) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
      std::cerr << "Failed to remove shared memory segment with shmid " << shmid
                << std::endl;
    } else {
      std::cout << "Removed shared memory segment with shmid " << shmid
                << std::endl;
    }
  }
  exit(0);
}

void *broadcast(void *arg) {
  std::string notification = "";
  while (true) {
    if (msgrcv(msgid, &msg, sizeof(msg.text), 1, IPC_NOWAIT) == -1) {
      // No new messages in queue
      usleep(100000);
      continue;
    }

    // Broadcast message to all connected clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_fds[i] != 0) {
        // just connected broadcast notifications
        if (client_dt[i].is_just_connected == 1) {
          notification += client_dt[i].name;
          notification += " joined the chat...";
          for (int j = 0; j < MAX_CLIENTS; j++) {
            if (client_fds[j] != 0 && i != j) {
              send(client_fds[j], notification.c_str(), notification.length(),
                   0);
            }
          }
          client_dt[i].is_just_connected = 0;
          notification = "";
        }
        // simple message broadcast
        if (send(client_fds[i], msg.text, strlen(msg.text), 0) == -1) {
          std::cerr << "Failed to send message to client\n";
        }
      }
      // just leaved broadcast notifications
      if (client_dt[i].is_just_leave == 1) {
        notification += client_dt[i].name;
        notification += " left the chat...";
        for (int j = 0; j < MAX_CLIENTS; j++) {
          if (client_fds[j] != 0 && i != j) {
            send(client_fds[j], notification.c_str(), notification.length(), 0);
          }
        }
        client_dt[i].is_just_leave = 0;
        notification = "";
      }
    }
  }
  pthread_exit(NULL);
}

int add_client(int client_socket, char *username) {
  int i;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_fds[i] == 0) {
      client_fds[i] = client_socket;
      strcpy(client_dt[i].name, username);
      client_dt[i].is_just_connected = 1;
      break;
    }
  }
  if (i == MAX_CLIENTS) {
    std::cerr << "Max clients reached\n";
    // close(client_socket);
    return 1;
  }
  return 0;
}

int clear_client() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_lost_cnnct[i] == -1 && client_fds[i] != 0) {
      // if (client_dt[i].is_connected == false && client_fds[i] != 0) {
      std::cout << "close connection with descriptor: " << client_fds[i]
                << std::endl;
      // close(client_fds[i]);
      client_lost_cnnct[i] = 0;
      client_dt[i].is_just_leave = 1;
      // client_dt[i].is_connected = true;
      client_fds[i] = 0;
      return 0;
    }
  }
  return 1;
}

void *handle_clients(void *arg) {
  // int new_socket;
  client_lost_cnnct = (int *)shmat(shmid_lost, NULL, 0);
  client_dt = (client_data *)shmat(shmid_clients, NULL, 0);
  signal(SIGCHLD, reaper);
  signal(SIGINT, cleanup);
  while (true) {
    // Accept new connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      std::cerr << "Connection accept error.\n";
      exit(EXIT_FAILURE);
    }

    std::cout << "new_client: " << new_socket << std::endl;

    // Get username
    char username[MAX_NAME_LENGTH];
    int recieved_bytes = recv(new_socket, username, MAX_NAME_LENGTH - 1, 0);
    username[recieved_bytes] = '\0';

    // Add new client to list of connected clients
    int is_max = add_client(new_socket, username);
    if (is_max) {
      continue;
    }

    // Fork new process to handle client messages
    pid_t pid = fork();
    if (pid == 0) {
      client_lost_cnnct = (int *)shmat(shmid_lost, NULL, 0);
      client_dt = (client_data *)shmat(shmid_clients, NULL, 0);
      close(server_fd);
      // recieve_messages(new_socket);
      while (true) {
        int nbytes = recv(new_socket, msg.text, sizeof(msg.text), 0);
        msg.text[nbytes] = '\0';
        //   // std::cout << "recieved_msg: " << msg.text << std::endl;
        if (nbytes == 0) {
          //     // Client disconnected
          for (int j = 0; j < MAX_CLIENTS; j++) {
            if (client_fds[j] == new_socket) {
              client_lost_cnnct[j] = -1;
            }
            // clear_client();
          }
        } else if (nbytes < 0) {
          // Error reading from client
          std::cerr << "Read error\n";
        }
        msg.type = 1;
        // send to parent process
        if (msgsnd(msgid, &msg, sizeof(msg.text), 0) == -1) {
          std::cerr << "Failed to send message to queue\n";
        }
      }
      exit(0);
    } else if (pid < 0) {
      // Fork error
      std::cerr << "Fork error\n";
      exit(EXIT_FAILURE);
    } else {
      // Parent process continues to accept new connections
      // close all connections
    }
  }
}
// close fd
void *handle_lost_connection(void *arg) {
  while (true) {
    clear_client();
  }
}

int main() {

  // int clnts_shmid;
  // Create shared memory segment for client file descriptors
  key = ftok("server", 'R');
  shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
  client_fds = (int *)shmat(shmid, NULL, 0);
  memset(client_fds, 0, SHM_SIZE);
  shmid_list.push_back(shmid);

  // Create message queue for broadcasting messages
  key = ftok("server", 'B');
  msgid = msgget(key, 0666 | IPC_CREAT);

  // Create shared memory segment for lost connection clients
  shmid_lost = shmget(IPC_PRIVATE, sizeof(int) * MAX_CLIENTS, IPC_CREAT | 0666);
  // memset(client_lost_cnnct, 0, SHM_SIZE);
  shmid_list.push_back(shmid_lost);
  client_lost_cnnct = (int *)shmat(shmid_lost, NULL, 0);

  shmid_clients =
      shmget(IPC_PRIVATE, sizeof(client_data) * MAX_CLIENTS, IPC_CREAT | 0666);
  shmid_list.push_back(shmid_clients);
  client_dt = (client_data *)shmat(shmid_clients, NULL, 0);

  // Create server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    std::cerr << "Socket creation error\n";
    exit(EXIT_FAILURE);
  }

  // Set server socket options
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    std::cerr << "Setsockopt error\n";
    exit(EXIT_FAILURE);
  }

  // Bind server socket to port
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "Bind error\n";
    exit(EXIT_FAILURE);
  }

  // Start listening for connections
  if (listen(server_fd, MAX_CLIENTS) < 0) {
    std::cerr << "Listen error\n";
    exit(EXIT_FAILURE);
  }

  pthread_t bcast_thread, handle_cl_thread, check_lost_connections_thread;
  // Start the broadcast thread
  if (pthread_create(&bcast_thread, NULL, broadcast, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&handle_cl_thread, NULL, handle_clients, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&check_lost_connections_thread, NULL,
                     handle_lost_connection, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }
  //   Wait for the threads to finish
  pthread_join(check_lost_connections_thread, NULL);
  pthread_join(bcast_thread, NULL);
  pthread_join(handle_cl_thread, NULL);

  signal(SIGINT, cleanup);
  signal(SIGCHLD, reaper);
  // while (true) {
  //   // Accept new connection
  //   if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
  //                            (socklen_t *)&addrlen)) < 0) {
  //     std::cerr << "Connection accept failure.\n";
  //     exit(EXIT_FAILURE);
  //   }
  //   std::cout << "new user: " << new_socket << std::endl;
  //   add_client(new_socket);
  //   // std::cout << "connected\n";

  //   // Add new client to list of connected clients
  //   // int i;
  //   // for (int i = 0; i < MAX_CLIENTS; i++) {
  //   //   if (client_fds[i] == 0) {
  //   //     client_fds[i] = new_socket;
  //   //     break;
  //   //   }
  //   // }
  //   // if (i == MAX_CLIENTS) {
  //   //   std::cerr << "Max clients reached\n";
  //   //   close(new_socket);
  //   //   continue;
  //   //   // char username[20];
  //   //   // recv(new_socket, username, 20, 0);
  //   //   // add_client(new_socket, username);
  //   //   // std::cout << username << " join the chat (" << new_socket << ")"
  //   //   //           << std::endl;
  //   // }

  //   // Fork new process to handle client messages
  //   pid_t pid = fork();
  //   if (pid == 0) {
  //     close(server_fd);
  //     while (true) {
  //       int nbytes = recv(new_socket, msg.text, sizeof(msg.text), 0);
  //       msg.text[nbytes] = '\0';
  //       // std::cout << "recieved_msg: " << msg.text << std::endl;
  //       if (nbytes == 0) {
  //         // Client disconnected
  //         for (int j = 0; j < MAX_CLIENTS; j++) {
  //           if (client_fds[j] == new_socket) {
  //             // client_fds[j] = 0;
  //             client_lost_cnnct[j] = -1;
  //             // break;
  //           }
  //         }
  //         // break;
  //       } else if (nbytes < 0) {
  //         // Error reading from client
  //         std::cerr << "Read error\n";
  //       }
  //       msg.type = 1;
  //       if (msgsnd(msgid, &msg, sizeof(msg.text), 0) == -1) {
  //         std::cerr << "Failed to send message to queue\n";
  //       }
  //       // exit(EXIT_FAILURE);
  //     }
  //     //   for (int j = 0; j < MAX_CLIENTS; j++) {
  //     //     if (client_fds[j] != 0 && client_fds[j] != new_socket) {
  //     //       if (send(client_fds[j], msg.text, strlen(msg.text), 0) <
  //     // 0)
  //     //   exit(0);
  //     // {
  //     //         // Error sending message to client
  //     //         cerr << "Send error\n";
  //     //         exit(EXIT_FAILURE);
  //     //       }
  //     //     }
  //     //   }
  //     // close(new_socket);
  //   } else if (pid < 0) {
  //     // Fork error
  //     std::cerr << "Fork error\n";
  //     exit(EXIT_FAILURE);
  //   } else {
  //     // Parent process continues to accept new connections
  //     //   close(new_socket);
  //   }
  // }

  // Detach and remove shared memory segment
  shmdt(client_fds);
  shmctl(shmid, IPC_RMID, NULL);
  shmctl(shmid_lost, IPC_RMID, NULL);

  // Remove message queue
  msgctl(msgid, IPC_RMID, NULL);
  pthread_exit(NULL);

  // close all opened connections
  // for (int i = 0; i < MAX_CLIENTS; i++) {
  //   if (client_fds[i] != 0) {
  //     close(client_fds[i]);
  //   }
  // }

  return 0;
}
