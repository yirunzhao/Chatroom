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

int online_user_count = 0;
int chatroom_count = 1;
// 用户id，注册的时候分配，以后操作的时候都要使用uid
static int Userid = 100;
static int Roomid = 10;
// 用一个结构记录客户端的连接
struct UserClient
{
    struct sockaddr_in addr; // 地址结构
    int fd;                  // 描述符
    int uid;                 // 用户的id,实际上不要也可以
    int roomid;              // 用户所属的聊天室
    char name[32];           // 用户名，初始化为uid
};
// 记录连接的用户
// 这个东西是固定的，有新的客户端进来有空即插
struct UserClient *clients[OPEN_MAX];

// 记录聊天室
struct Chatroom
{
    int roomid;
    char password[50];
};
// 记录所有的聊天室
struct Chatroom *rooms[OPEN_MAX + 1];

void add_to_room_list(struct Chatroom *room)
{
    for (int i = 0; i < OPEN_MAX + 1; i++)
    {
        if (!rooms[i])
        {
            rooms[i] = room;
            break;
        }
    }
    chatroom_count++;
}
void delete_from_room_list(int roomid)
{
    for (int i = 0; i < OPEN_MAX + 1; i++)
    {
        if (rooms[i])
        {
            if (rooms[i]->roomid == roomid)
            {
                rooms[i] = NULL;
                break;
            }
        }
    }
    chatroom_count--;
}
void add_to_client_list(struct UserClient *cli)
{
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (!clients[i])
        {
            clients[i] = cli;
            break;
        }
    }
    online_user_count++;
}
void delete_from_client_list(int uid)
{
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                clients[i] = NULL;
                break;
            }
        }
    }
    online_user_count--;
}
// help文档
void send_help(int fd)
{
    char msg[MAXLINE];
    strcat(msg, "** This is a chatroom made by WHU.CS.Ryan, see more details on my github: https://github.com/yirunzhao/Chatroom **\n");
    strcat(msg, "** This chatroom is the final assigment of Linuusernamex Network Programming **\n");
    strcat(msg, "** Use 'zyrctrm login `username` `password`' to sign in **\n");
    strcat(msg, "** Use 'zyrctrm register `username` `password`' to sign up **\n");
    strcat(msg, "** Use 'zyrctrm list -r' to get all online chatrooms **\n");
    strcat(msg, "** Use 'zyrctrm list -u' to get all online users **\n");
    strcat(msg, "** Use 'zyrctrm leave' to leave current chatroom **\n");
    strcat(msg, "** Use 'zyrctrm createrm `roomid` `roompwd` to create a chatroom **\n");
    strcat(msg, "** Use 'zyrctrm enter `roomid` `roompwd` to enter a chatroom **\n");
    strcat(msg, "** Use 'zyrctrm sendtouser `userid` `message` to send messages to a certain user **\n");
    strcat(msg, "** Use 'zyrctrm send `message` to send messages to your current chatroom **\n");
    Write(fd, msg, strlen(msg));
}
// 给同一个聊天室的所有用户发消息
void send_to_all(char *msg, struct UserClient *cli)
{
    char *all;
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (clients[i]->roomid == cli->roomid)
            {
                sprintf(all, "[%s] says:%s", cli->name, msg);
                Write(clients[i]->fd, all, strlen(all));
            }
        }
    }
}
// 给特定用户发消息
void send_to_certain(char *msg, struct UserClient *cli, int uid)
{
    char *all;
    if (cli->uid == uid)
    {
        sprintf(all, "You can't send messages to yourself!\n");
        Write(cli->fd, all, strlen(all));
        return;
    }
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (clients[i]->uid == uid)
            {
                sprintf(all, "[%s] sends a private message to you:%s", cli->name, msg);
                Write(clients[i]->fd, all, strlen(all));
            }
        }
    }
}
// list -u 发送所有在线用户
void send_online_users(int fd)
{
    char all[100];
    sprintf(all, "Username\tUserID\tUserRoom\tTotal Online:%d\n", online_user_count);
    Write(fd, all, strlen(all));
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            sprintf(all, "%s\t%d\t%d\n", clients[i]->name, clients[i]->uid, clients[i]->roomid);
            Write(fd, all, strlen(all));
        }
    }
}
// list -r 发送所有开设的聊天室
void send_open_rooms(int fd)
{

}
int is_chatroom_empty(int roomid)
{
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (clients[i]->roomid == roomid)
            {
                return 0;
            }
        }
    }
    // 都不是则返回true
    return 1;
}
struct Chatroom *get_room(int rmid)
{
    for (int i = 0; i < OPEN_MAX + 1; i++)
    {
        if (rooms[i])
        {
            if (rooms[i]->roomid == rmid)
            {
                return rooms[i];
            }
        }
    }
    return NULL;
}
int join_chatroom(int rmid, char *pwd, struct UserClient *cli)
{
    struct Chatroom *room;
    room = get_room(rmid);
    if (room != NULL)
    {
        // 如果密码相同
        if (strcmp(room->password, pwd) == 0)
        {
            cli->roomid = rmid;
            return 1;
        }
        else
        {
            return 0;
        }
    }
}
void trim_string(char *str)
{
    int len = strlen(str);
    if (str[len - 1] == '\n')
    {
        len--;
        str[len] = 0;
    }
}
void strip_newline(char *s)
{
    while (*s != '\0')
    {
        if (*s == '\r' || *s == '\n')
        {
            *s = '\0';
        }
        s++;
    }
}
struct UserClient *get_client(int fd)
{
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i]->fd == fd)
        {
            return clients[i];
        }
    }
    return NULL;
}

int main(void)
{
    int i, num = 0;                          //num是连接数量，i是循环变量
    int n;                                   // n是读出的字节数
    int server_fd, connect_fd, sock_fd;      // 三个描述符
    ssize_t nready, epfd, ret;               // epoll的相关，返回值
    char buf[MAXLINE], str[INET_ADDRSTRLEN]; // 接受客户端发来的信息
    char *command, *param;                   // 指令、指令的参数
    char message[MAXLINE];                   // 发送的消息
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
                struct UserClient *cli = (struct UserClient *)malloc(sizeof(struct UserClient));
                cli->addr = client_addr;
                cli->fd = connect_fd;
                cli->roomid = 0;
                cli->uid = Userid++;
                sprintf(cli->name, "user %d", cli->uid);
                add_to_client_list(cli);

                // 公屏，聊天室编号为0
                struct Chatroom *rm = (struct Chatroom *)malloc(sizeof(struct Chatroom));
                rm->roomid = 0;
                // 其实不需要密码也可以
                sprintf(rm->password, "%s", "123456");
                add_to_room_list(rm);
            }
            // 如果是数据读写事件
            else
            {
                // 得到对应的描述符
                sock_fd = ep[i].data.fd;
                struct UserClient *cli = get_client(sock_fd);
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
                    delete_from_client_list(cli->uid);
                }
                else if (n < 0)
                {
                    perror("read n < 0 error:");
                    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, sock_fd, NULL);
                    Close(sock_fd);
                    delete_from_client_list(cli->uid);
                }
                else
                {
                    // buf 里面就是客户端发过来的指令了
                    // send hello world
                    printf("接受到客户端命令:%s\n", buf);
                    strip_newline(buf);
                    if (strlen(buf) == 0)
                    {
                        continue;
                    }
                    // trim_string(command);
                    // 截取第一个
                    command = strtok(buf, " ");
                    printf("|截取的%s|\n", command);

                    // 发送给全部
                    if (strcmp(command, "send") == 0)
                    {
                        // 截取第二个
                        param = strtok(NULL, " ");
                        if (param)
                        {
                            // sprintf(message,"%s",param);
                            while (param != NULL)
                            {
                                strcat(message, " ");
                                strcat(message, param);
                                param = strtok(NULL, " ");
                            }
                            printf("message is %s", message);
                            strcat(message, "\n");
                            send_to_all(message, cli);
                            // 发送完之后应该清零
                            memset(message, 0, sizeof(message));
                        }
                    }
                    // 发送给特定用户
                    if (strcmp(command, "sendtouser") == 0)
                    {
                        // sendtouser uid msg
                        param = strtok(NULL, " ");
                        int sendto_uid = atoi(param);
                        if (param)
                        {
                            while (param != NULL)
                            {
                                strcat(message, " ");
                                param = strtok(NULL, " ");
                                if (param != NULL)
                                    strcat(message, param);
                            }
                            send_to_certain(message, cli, sendto_uid);
                            memset(message, 0, sizeof(message));
                        }
                    }
                    // 查看帮助
                    if (strcmp(command, "help") == 0)
                    {
                        send_help(sock_fd);
                    }
                    // list操作
                    if (strcmp(command, "list") == 0)
                    {
                        param = strtok(NULL, " ");
                        // 查看在线用户
                        if (strcmp(param, "-u") == 0)
                        {
                            send_online_users(sock_fd);
                        }
                        // 查看聊天室
                        if (strcmp(param, "-r") == 0)
                        {
                        }
                    }
                    // 如果是创建房间
                    // createrm password
                    if (strcmp(command, "createrm") == 0)
                    {
                        Write(STDOUT_FILENO, "fuck", sizeof("test"));
                        param = strtok(NULL, " ");
                        if (param != NULL)
                        {
                            // 拿到密码
                            struct Chatroom *rm = (struct Chatroom *)malloc(sizeof(struct Chatroom));
                            rm->roomid = Roomid++;
                            sprintf(rm->password, "%s", param);
                            // 加入到list中
                            add_to_room_list(rm);
                            // 把用户的房间切换
                            cli->roomid = rm->roomid;
                        }
                    }
                    // 进入聊天室
                    // join roomid roompwd
                    if (strcmp(command, "join") == 0)
                    {
                        int rmid;
                        char pwd[50];
                        param = strtok(NULL, " "); // param此时是roomid
                        if (param != NULL)
                        {
                            rmid = atoi(param);
                            param = strtok(NULL, " "); // param此时是pwd
                            if (param != NULL)
                            {
                                sprintf(pwd, "%s", param);
                                int ret = join_chatroom(rmid, pwd, cli);
                                if (ret == 1)
                                    send_to_all(" I have joined the chatroom!\n", cli);
                                else
                                    Write(sock_fd, "Wrong password or room id\n", strlen("Wrong password or room id\n"));
                            }
                        }
                    }
                    // 离开聊天室
                    if (strcmp(command, "leave") == 0)
                    {
                        send_to_all(" I have left chatroom\n", cli);
                        int tempid = cli->roomid;
                        // 切换到公屏
                        cli->roomid = 0;
                        // 判断一下聊天室的人数，如果没有人了就关闭
                        if (is_chatroom_empty(cli->roomid))
                        {
                            delete_from_room_list(tempid);
                        }
                    }
                }
            }
        }
    }

    return 0;
}