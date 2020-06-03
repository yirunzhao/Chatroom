/*
    Author: WHU.CS.Ryan
    Date: 2020.6
    
*/
// 暂时不考虑互斥问题
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
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "wrap.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define OPEN_MAX 100
#define MAXLINE 8192

int user_count;
// 用户id，注册的时候分配，以后操作的时候都要使用uid
static int userid = 100;
// 用一个结构记录客户端的连接
struct UserClient{
    struct sockaddr_in addr;        // 地址结构
    int fd;                         // 描述符
    int uid;                        // 用户的id,实际上不要也可以
    int gid;                        // 用户所属的聊天室
    char name[32];                  // 用户名，初始化为uid
};

// 记录连接的用户
// 这个东西是固定的，有新的客户端进来有空即插
struct UserClient *clients[OPEN_MAX];
// 给客户端发送用户信息
void send_userinfo()
{
}
// 给客户端发送聊天室信息
void send_chatroominfo()
{
}

void add_to_client_list(struct UserClient *cli)
{
    for(int i = 0; i < OPEN_MAX; i++){
        if(!clients[i]){
            clients[i] = cli;
            break;
        }
    }
}
void delete_from_client_list(int uid)
{
    for(int i = 0; i < OPEN_MAX; i++){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                break;
            }
        }
    }
}


int main(void)
{
    int i, num = 0;                     //num是连接数量，i是循环变量
    int n;                              // n是读出的字节数
    int server_fd, connect_fd, sock_fd; // 三个描述符
    ssize_t nready, epfd, ret;          // epoll的相关，返回值
    char buf[MAXLINE], str[INET_ADDRSTRLEN];

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    // epoll的临时变量，用于挂载到红黑树
    struct epoll_event temp, ep[OPEN_MAX];

    // 服务端监听的socket
    server_fd = Socket(AF_INET, SOCK_STREAM, 0);

    // 设置端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 设置服务端的地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_addr.sin_addr.s_addr = htonl(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // bind绑定
    Bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Listen监听
    Listen(server_fd, 50);

    // 创建epoll的红黑树
    epfd = epoll_create(OPEN_MAX);
    if (epfd == -1)
        perr_exit("epoll create error");

    // 挂载监听的服务器socket
    temp.data.fd = server_fd;
    temp.events = EPOLLIN;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &temp);
    if (ret == -1)
        perr_exit("挂载监听fd失败");

    // 进行阻塞接受
    while (1)
    {
        nready = epoll_wait(epfd, ep, OPEN_MAX, -1);
        if (nready == -1)
            perr_exit("epoll wait错误");

        for (i = 0; i < nready; i++)
        {
            // 如果是建立连接的读事件
            if (ep[i].data.fd == server_fd)
            {
                // 连接
                client_len = sizeof(client_addr);
                connect_fd = Accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                printf("received from %s at port %d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port));
                printf("cfd %d --- client %d\n", connect_fd, ++num);

                // 挂载到树上
                temp.data.fd = connect_fd;
                temp.events = EPOLLIN;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connect_fd, &temp);
                if (ret == -1)
                    perr_exit("挂载建立连接节点失败");

                // 把用户添加进数组中去
                struct UserClient *cli = (struct UserClient*)malloc(sizeof(struct UserClient));
                cli->addr = client_addr;
                cli->fd = connect_fd;
                cli->gid = 0;
                cli->uid = userid++;
                sprintf(cli->name,"user %d",cli->uid);
                add_to_client_list(cli);

            }
            // 如果是数据读写事件
            else
            {
                // 得到对应的描述符
                sock_fd = ep[i].data.fd;
                // 得到用户发送的信息，指令判断就写在这里了
                n = Read(sock_fd, buf, MAXLINE);

                // 沙都没有，关闭连接
                if (n == 0)
                {
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sock_fd, NULL);
                    if (ret == -1)
                        perr_exit("n=0关闭失败");

                    Close(sock_fd);
                    printf("Client[%d] closed connection\n", sock_fd);
                }
                else if (n < 0)
                {
                    perror("read n < 0 error:");
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sock_fd, NULL);
                    Close(sock_fd);
                }
                else
                {
                    // 处理数据，应该对数据进行编码分类，不然不知道该干什么
                    // Writen(sock_fd,buf,strlen(buf));
                    // buf 里面就是客户端发过来的指令了
                    printf("接受到客户端命令:%s\n",buf);
                    
                }
            }
        }
    }

    return 0;
}