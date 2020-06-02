#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <error.h>
#include <ctype.h>
#include "wrap.h"

#define MAXLINE 8192
#define SERV_PORT 8888
#define OPEN_MAX 5000

int main(int argc, char *argv[])
{
    int i, listenfd, connfd, sockfd;
    int n, num = 0;
    ssize_t nready, efd, res;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t clilen;

    struct sockaddr_in cliaddr, servaddr;
    // epoll的结构体，tep是临时的，要往上面挂的，ep是数组
    struct epoll_event tep, ep[OPEN_MAX];

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //端口服用
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, 20);

    // 创建epoll模型红黑书
    efd = epoll_create(OPEN_MAX);
    if (efd == -1)
        perr_exit("Epoll create");

    // 把listen fd挂到树上去
    tep.events = EPOLLIN;
    tep.data.fd = listenfd;
    res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    if (res == -1)
        perr_exit("epoll ctr");

    while (1)
    {
        // 阻塞模式，返回的是满足的数量
        nready = epoll_wait(efd, ep, OPEN_MAX, -1);
        if (nready == -1)
            perr_exit("wait");

        for (i = 0; i < nready; i++)
        {
            // 建立连接的读事件
            if (ep[i].data.fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

                printf("received from %s at port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
                printf("cfd %d --- client %d\n", connfd, ++num);

                tep.events = EPOLLIN;
                tep.data.fd = connfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                if (res == -1)
                    perr_exit("ctl 2");
            }
            // 数据读写事件
            else
            {
                sockfd = ep[i].data.fd;
                n = Read(sockfd, buf, MAXLINE);

                // 客户端关闭连接
                if (n == 0)
                {
                    res = epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
                    if(res == -1)
                        perr_exit("ctl 3");
                    Close(sockfd);
                    printf("Client[%d] closed connection\n",sockfd);
                } 
                // 出错
                else if(n < 0) {
                    perror("read n < 0 error:");
                    res = epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
                    Close(sockfd);
                }
                else{
                    for(i = 0; i < n; i++)
                        buf[i] = toupper(buf[i]);
                    Write(STDOUT_FILENO,buf,n);
                    Writen(sockfd,buf,n);
                }
            }
        }
    }
}