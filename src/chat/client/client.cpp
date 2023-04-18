#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

const int MAX_MESSAGE_SIZE = 1024;

int client_socket;
char message[MAX_MESSAGE_SIZE];

void *receive_messages(void *arg) {
  int bytes_read;
  while (true) {
    // Receive messages from the server
    bytes_read = read(client_socket, message, MAX_MESSAGE_SIZE);
    if (bytes_read == 0) {
      std::cout << "Server closed the connection" << std::endl;
      break;
    } else if (bytes_read < 0) {
      perror("Error reading from socket");
      break;
    } else {
      message[bytes_read] = '\0';
      std::cout << "server: " << message << std::endl;
    }
  }
  pthread_exit(NULL);
}

void *send_messages(void *arg) {
  while (true) {
    // Get input from the user
    if (fgets(message, MAX_MESSAGE_SIZE, stdin) != 0) {
      // Send the message to the server
      if ((send(client_socket, message, strlen(message), 0)) < 0) {
        perror("Error sending message");
        break;
      }
    }
  }
  pthread_exit(NULL);
}

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
  // int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    std::cerr << "Error creating socket" << std::endl;
    return 1;
  }
  std::cout << "My socket: " << client_socket << std::endl;

  // Configure the server address
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr = ip_address;
  server_address.sin_port = htons(port);

  if ((connect(client_socket, (struct sockaddr *)&server_address,
               sizeof(server_address))) == -1) {
    perror("connect");
    exit(1);
  }

  pthread_t receive_thread, send_thread;
  // Start the receive_messages thread
  if (pthread_create(&receive_thread, NULL, receive_messages, NULL) < 0) {
    perror("Error creating receive thread");
    exit(EXIT_FAILURE);
  }

  // Start the send_messages thread
  if (pthread_create(&send_thread, NULL, send_messages, NULL) < 0) {
    perror("Error creating send thread");
    exit(EXIT_FAILURE);
  }

  // Wait for the threads to finish
  pthread_join(receive_thread, NULL);
  pthread_join(send_thread, NULL);

  // Close the client socket and exit
  close(client_socket);
  pthread_exit(NULL);
}

// #include <stdio.h>
// #include <sys/socket.h>
// #include <sys/types.h>

// #include <errno.h>
// #include <netdb.h>
// #include <netinet/in.h>

// #include <arpa/inet.h>
// #include <iostream>
// #include <strings.h>

// #include <fcntl.h> // для функции fcntl()
// #include <unistd.h>
// #include <vector>

// #include <signal.h>
// #include <sys/wait.h>

// #include <cstring>
// #include <fstream>
// #include <pthread.h>

// #define MAX_CLIENTS 10
// #define MAX_MESSAGE_LENGTH 1024
// #define MAX_NAME_LENGTH 20
// // ver 5
// int main(int argc, char **argv) {

//   int server_socket;
//   sockaddr_in server_address;
//   hostent *hp;

//   char buffer[MAX_MESSAGE_LENGTH];
//   ssize_t message_size;

//   if (argc != 3) {
//     printf("Error: enter server's Port and your Username!\n");
//     exit(1);
//   }

//   // Создаем сокет
//   if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//     std::cerr << "Error creating socket\n";
//     return 1;
//   }

//   bzero((char *)&server_address, sizeof(server_address));

//   server_address.sin_family = AF_INET;

//   hp = gethostbyname("localhost");

//   bcopy(hp->h_addr, &server_address.sin_addr, hp->h_length);

//   server_address.sin_port = htons(atoi(argv[1]));

//   if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
//     std::cerr << "Error invalid address\n";
//     return 1;
//   }

//   // Подключаемся к серверу
//   if (connect(server_socket, (sockaddr *)&server_address,
//               sizeof(server_address)) < 0) {
//     std::cerr << "Error connecting to server\n";
//     return 1;
//   }

//   // Получаем приветственное сообщение
//   // if ((message_size = recv(server_socket, buffer, MAX_MESSAGE_LENGTH, 0))
//   <=
//   //     0) {
//   //   std::cerr << "Error receiving message\n";
//   //   return 1;
//   // }

//   // buffer[message_size] = '\0';
//   // std::cout << buffer << std::endl << std::endl;

//   std::string name = argv[2];
//   // send(server_socket, name.c_str(), name.size(), 0);

//   // Отправляем сообщения серверу
//   while (true) {

//     std::string message;
//     std::string input_message;

//     fd_set read_fds;

//     struct timeval timeout;

//     FD_ZERO(&read_fds);
//     FD_SET(server_socket, &read_fds);
//     FD_SET(STDIN_FILENO, &read_fds);

//     timeout.tv_sec = 0;
//     timeout.tv_usec = 1000;

//     int activity = select(server_socket + 1, &read_fds, NULL, NULL,
//     &timeout);

//     if (activity < 0) {
//       std::cerr << "Error selecting\n";
//       return 1;
//     }

//     if (activity == 0)
//       continue;

//     if (FD_ISSET(server_socket, &read_fds)) {
//       if ((message_size = recv(server_socket, buffer, MAX_MESSAGE_LENGTH, 0))
//       <=
//           0) {
//         std::cerr << "Error receiving message\n";
//         return 1;
//       }

//       buffer[message_size] = '\0';
//       std::cout << buffer << std::endl << std::endl;
//     }

//     if (FD_ISSET(STDIN_FILENO, &read_fds)) {
//       std::getline(std::cin, input_message);
//       std::cout << std::endl;

//       if (input_message == "/quit") {
//         break;
//       }

//       message = name + ": " + input_message;
//       send(server_socket, message.c_str(), message.size(), 0);
//     }
//   }

//   // Закрываем соединение
//   close(server_socket);

//   return 0;
// }
