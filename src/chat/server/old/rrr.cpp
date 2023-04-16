#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include <iostream>
#include <strings.h>

#include <unistd.h>

#include <signal.h>
#include <sys/wait.h>

// Этот способ подразумевает создание дочернего процесса для обслуживания
// каждого нового клиента. При этом родительский процесс занимается только
// прослушиванием порта и приёмом соединений. Чтобы добиться такого поведения,
// сразу после accept сервер вызывает функцию fork для создания дочернего
// процесса. Далее анализируется значение, которое вернула эта функция. В
// родительском процессе оно содержит идентификатор дочернего, а в дочернем
// процессе равно нулю. Используя этот признак, мы переходим к очередному вызову
// accept в родительском процессе, а дочерний процесс обслуживает клиента и
// завершается (_exit).

#define BUFFLEN 81

// int *test_buff = new int[10];

//Прием сообщения от клиента и вывод его на экран
// int buff_work(int client_fd) {
//   char *buf = new char[BUFFLEN];
//   int msg_length;
//   bzero(buf, BUFFLEN);

//   //   std::cout << std::endl;
//   //   test_buff[atoi(buf)] = 5;
//   if (msg_length < 0) {
//     perror("RECV FAILURE\n");
//     return -1;
//   } else if (msg_length == 0) {
//     printf("\n\t-----NO MESSAGES FOR RECV-----\n");
//     return 1;
//   }
//   send(client_fd, "hello", 6, 0);
//   msg_length = recv(client_fd, buf, BUFFLEN, 0);
//   //   std::cout << "Process_buffer: ";
//   //   for (int i = 0; i < 10; i++) {
//   //     std::cout << test_buff[i] << ",";
//   //   }

//   printf("SERVER: CLIENT SOCKET (FD) - %d\n", client_fd);
//   printf("SERVER: MESSAGE LEN - %d\n", msg_length);
//   printf("SERVER: MESSAGE - %s\n\n", buf);
//   return 0;
// }

//Прием сообщения от клиента и вывод его на экран
int buff_work(int client_fd) {
  char *buf = new char[BUFFLEN];
  int msg_length;
  bzero(buf, BUFFLEN);

  msg_length = recv(client_fd, buf, BUFFLEN, 0);
  if (msg_length < 0) {
    perror("RECV FAILURE\n");
    return -1;
  } else if (msg_length == 0) {
    printf("\n\t-----NO MESSAGES FOR RECV-----\n");
    return 1;
  }
  send(client_fd, "hello", 6, 0);

  printf("SERVER: CLIENT SOCKET (FD) - %d\n", client_fd);
  printf("SERVER: MESSAGE LEN - %d\n", msg_length);
  printf("SERVER: MESSAGE - %s\n\n", buf);
  return 0;
}

void reaper(int signal) {
  int status;
  while (wait3(&status, WNOHANG, (rusage *)0) >= 0)
    ;
}

int main() {

  int server_fd, client_fd, addr_length, child_pid;
  sockaddr_in server_addr;

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Server can't open socket for TCP\n");
    exit(EXIT_FAILURE);
  }
  bzero((char *)&server_addr, sizeof(server_addr));

  // Zero initialization
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = 0;

  if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Server binding failure\n");
    exit(EXIT_FAILURE);
  }

  addr_length = sizeof(server_addr);
  // //В поля записывает адрес (и сколько места под него выделено), к которому
  // привязан сокет
  if (getsockname(server_fd, (sockaddr *)&server_addr,
                  (socklen_t *)&addr_length)) {
    perror("Server binding failure\n");
    exit(EXIT_FAILURE);
  }

  printf("SERVER: PORT - %d\n", ntohs(server_addr.sin_port));
  // second agrument is a backlog - how many connections can be waiting for this
  // socket simultaneously
  listen(server_fd, 5);

  // recieve client's messages
  // If PID < 0; /* Ошибка при порождении процесса. */

  //Процесс-потомок и процесс-родитель получают разные коды возврата после
  //вызова fork(). If PID > 0; /* Это процесс-родитель, который успешно создал
  // процесс-потомок. Значение записанное в PID – идентификатор созданного
  //процесса-потомка. */ If PID == 0; /* Это процесс-потомок.
  // for (int i = 0; i < 10; i++)
  //   test_buff = 0;
  signal(SIGCHLD, reaper);
  while (1) {
    if ((client_fd = accept(server_fd, 0, 0)) < 0) {
      perror("Incorrect client's socket\n");
      exit(EXIT_FAILURE);
    }

    child_pid = fork();
    if (child_pid < 0) {
      perror("Child-process creation failure\n");
      exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
      close(server_fd);
      int flag = 0;
      while (true) {
        std::cout << "pesos\n";
        flag = buff_work(client_fd);
        if (flag != 0)
          break;
      }
      close(client_fd);
      exit(0);
    } else {
      printf("\t-----CHILD_PROCESS PID: %d -----\n\n", child_pid);
      close(client_fd);
    }
  }
  close(server_fd);
  return 0;
}