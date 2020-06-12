#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "wrap.h"

// ----定义socket相关的宏----
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define OPEN_MAX 100
#define MAXLINE 8192
// ----end----

int main()
{
    struct sockaddr_in server_addr;
    char buf[MAXLINE];
    char command[100];
    char test[100];
    int sock_fd, n;
    sock_fd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVER_PORT);

    Connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    n = Read(sock_fd, buf, MAXLINE);
    if (n != 0)
        Write(STDOUT_FILENO, buf, strlen(buf));
    bzero(buf, sizeof(buf));
    while (1)
    {
        printf(">> ");
        // 获得指令
        fgets(command, 100, stdin);
        // 发送过去
        Write(sock_fd, command, strlen(command));

        n = Read(sock_fd, buf, MAXLINE);
        if (n != 0)
            Write(STDOUT_FILENO, buf, strlen(buf));
            
        bzero(buf, sizeof(buf));
    }
    Close(sock_fd);
}