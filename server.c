/*
    Author: WHU.CS.Ryan
    Date: 2020.6
*/
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
#include <mysql/mysql.h>

// ----定义mysql连接的宏----
#define HOST "localhost"
#define USER "root"
#define PASSWORD "123456"
#define DATABASE "db_chatroom"
// ----end----

// ----定义socket相关的宏----
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define OPEN_MAX 100
#define MAXLINE 8192
// ----end----

// ----定义一些全局变量----
int online_user_count = 0;
int chatroom_count = 0;
// 用户id，注册的时候分配，以后操作的时候都要使用uid
static int Userid = 100;
static int Roomid = 10;
// ----end----

// ----定义客户端和聊天室的结构体----
// 记录客户端的
struct UserClient
{
    struct sockaddr_in addr; // 地址结构
    int fd;                  // 描述符
    int uid;                 // 用户的id,实际上不要也可以
    int roomid;              // 用户所属的聊天室
    char name[32];           // 用户名，初始化为uid
    char password[20];       // 用户密码
    int islogin;             // 判断是否登陆
};
// 记录连接的用户
// 这个东西是固定的，有新的客户端进来有空即插
struct UserClient *clients[OPEN_MAX];

// 记录聊天室
struct Chatroom
{
    int roomid;        // id
    char password[50]; // 密码
    int numbers;       // 该聊天室的人数
};
// 记录所有的聊天室
struct Chatroom *rooms[OPEN_MAX + 1];
// ----end----

// --------所有的函数声明--------
int query(MYSQL *conn, char *sql);                                // mysql查询
void add_to_room_list(struct Chatroom *room);                     // 把聊天室加入list
void delete_from_room_list(int roomid);                           // 把聊天室删除
void add_to_client_list(struct UserClient *cli);                  //
void delete_from_client_list(int uid);                            //
void send_help(int fd);                                           // help文档
void send_to_all(char *msg, struct UserClient *cli);              // 给同一个聊天室的所有用户发消息
void send_to_certain(char *msg, struct UserClient *cli, int uid); // 给特定用户发消息
void send_online_users(int fd);                                   // list -u 发送所有在线用户
void send_open_rooms(int fd);                                     // list -r 发送所有开设的聊天室
struct Chatroom *get_room(int rmid);                              // 根据房间id获取房间
int join_chatroom(int rmid, char *pwd, struct UserClient *cli);
void strip_newline(char *s);
struct UserClient *get_client(int fd);
// --------end------------
int query(MYSQL *conn, char *sql)
{
    int ret;
    MYSQL_RES *res;
    MYSQL_FIELD *field;
    MYSQL_ROW res_row;
    int row, col;
    int i, j;

    ret = mysql_query(conn, sql);
    if (ret)
    {
        printf("error\n");
        mysql_close(conn);
        return -1;
    }
    else
    {
        res = mysql_store_result(conn);
        if (res)
        {
            col = mysql_num_fields(res);
            row = mysql_num_rows(res);
            printf("查询到 %d 行\n", row);
            for (i = 0; field = mysql_fetch_field(res); i++)
                printf("%10s ", field->name);
            printf("\n");
            for (i = 1; i < row + 1; i++)
            {
                res_row = mysql_fetch_row(res);
                for (j = 0; j < col; j++)
                    printf("%10s ", res_row[j]);
                printf("\n");
            }
        }
        return row;
    }
}

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
                free(rooms[i]);
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
                free(clients[i]);
                clients[i] = NULL;
                break;
            }
        }
    }
    online_user_count--;
}

void send_help(int fd)
{
    char msg[MAXLINE];
    strcat(msg, "** This is a chatroom made by WHU.CS.Ryan, see more details on my github: https://github.com/yirunzhao/Chatroom **\n");
    strcat(msg, "** This chatroom is the final assigment of Linuusernamex Network Programming **\n");
    strcat(msg, "** Use ' login `username` `password`' to sign in **\n");
    strcat(msg, "** Use ' register `username` `password`' to sign up **\n");
    strcat(msg, "** Use ' list -r' to get all online chatrooms **\n");
    strcat(msg, "** Use ' list -u' to get all online users **\n");
    strcat(msg, "** Use ' leave' to leave current chatroom **\n");
    strcat(msg, "** Use ' createrm `roomid` `roompwd` to create a chatroom **\n");
    strcat(msg, "** Use ' enter `roomid` `roompwd` to enter a chatroom **\n");
    strcat(msg, "** Use ' sendtouser `userid` `message` to send messages to a certain user **\n");
    strcat(msg, "** Use ' send `message` to send messages to your current chatroom **\n");
    Write(fd, msg, strlen(msg));
}

void send_to_all(char *msg, struct UserClient *cli)
{
    char *all_msg;
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (clients[i]->roomid == cli->roomid)
            {
                sprintf(all_msg, "[%s] says:%s", cli->name, msg);
                Write(clients[i]->fd, all_msg, strlen(all_msg));
            }
        }
    }
}

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
                sprintf(all, "[%s] sends a private message to you:%s\n", cli->name, msg);
                Write(clients[i]->fd, all, strlen(all));
            }
        }
    }
}

void send_online_users(int fd)
{
    char all[100];
    sprintf(all, "|Username|\t|UserID|\t|UserRoom|\t|Total Online:%d|\n", online_user_count);
    Write(fd, all, strlen(all));
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (clients[i])
        {
            if (strlen(clients[i]->name) >= 8)
                sprintf(all, "|%s|\t|%d|\t\t|%d|\n", clients[i]->name, clients[i]->uid, clients[i]->roomid);
            else
                sprintf(all, "|%s|\t\t|%d|\t\t|%d|\n", clients[i]->name, clients[i]->uid, clients[i]->roomid);
            Write(fd, all, strlen(all));
        }
    }
}

void send_open_rooms(int fd)
{
    // char all[100];
    char *all;
    sprintf(all, "|Room id|\t|User count|\t|Total Rooms:%d|\n", chatroom_count);
    Write(fd, all, strlen(all));
    for (int i = 0; i < OPEN_MAX; i++)
    {
        if (rooms[i] != NULL)
        {
            if (rooms[i]->roomid != 0)
            {
                memset(all, 0, sizeof(all));
                sprintf(all, "|%d|\t\t|%d|\t\n", rooms[i]->roomid, rooms[i]->numbers);
                Write(fd, all, strlen(all));
            }
        }
    }
}
/*
int is_chatroom_empty(int roomid)
{
    for (int i = 0; i < OPEN_MAX + 1; i++)
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
暂时没用
*/
struct Chatroom *get_room(int rmid)
{
    for (int i = 0; i < OPEN_MAX + 1; i++)
    {
        if (rooms[i] != NULL)
        {
            if (rooms[i]->roomid == rmid)
            {
                // char s[100];
                // sprintf(s, "before rmid:%d\n", rooms[i]->roomid);
                // Write(STDOUT_FILENO, s, strlen(s));
                return rooms[i];
            }
        }
    }
    return NULL;
}
int join_chatroom(int rmid, char *pwd, struct UserClient *cli)
{
    struct Chatroom *room, *temp;
    room = get_room(rmid);
    // TMD这个地方必须复制，不然下面就会出错
    char *password;
    strcpy(password, room->password);

    if (room != NULL)
    {
        if (strcmp(password, pwd) == 0)
        {
            cli->roomid = rmid;
            room->numbers++;
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        Write(STDOUT_FILENO, "FUCK", strlen("FCUK"));
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
    char *hello = "Welcome to WHU.CS.Ryan's Chatroom!\n"; // 欢迎信息
    char *hint = "Wrong command! Please use 'help' to get all commands!\n";
    char *need_login = "You have to login first!\n";
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
    temp.events = EPOLLIN | EPOLLET;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &temp);
    if (ret == -1)
        perr_exit("挂载监听fd失败");

    // 建立公屏聊天室
    // 公屏，聊天室编号为0
    struct Chatroom *rm = (struct Chatroom *)malloc(sizeof(struct Chatroom));
    rm->roomid = 0;
    rm->numbers = -1;
    // 其实不需要密码也可以
    sprintf(rm->password, "%s", "123456");
    add_to_room_list(rm);

    // mysql初始化
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, HOST, USER, PASSWORD, DATABASE, 0, NULL, 0))
    {
        printf("ok");
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

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
                cli->islogin = 0; // 没有登陆
                sprintf(cli->name, "user %d", cli->uid);
                sprintf(cli->password,"123");   // 初始密码，其实没有意义
                add_to_client_list(cli);
                
                Write(connect_fd,hello,strlen(hello));
            }
            // 如果是数据读写事件
            else
            {
                // 得到对应的描述符
                sock_fd = ep[i].data.fd;
                // Write(sock_fd,"zyr chatroom >> ",strlen("zyr chatroom >> "));
                // cli就是当前的用户
                struct UserClient *cli = get_client(sock_fd);
                // 得到用户发送的信息，指令判断就写在这里了
                memset(buf, 0, MAXLINE);
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
                    else if (strcmp(command, "sendtouser") == 0)
                    {
                        if (cli->islogin == 1)
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
                        else
                        {
                            Write(sock_fd, "You have already left!\n", strlen("You have already left!\n"));
                        }
                    }
                    // 查看帮助
                    else if (strcmp(command, "help") == 0)
                    {
                        send_help(sock_fd);
                    }
                    // list操作
                    else if (strcmp(command, "list") == 0)
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
                            send_open_rooms(sock_fd);
                        }
                    }
                    // 如果是创建房间
                    // createrm password
                    else if (strcmp(command, "createrm") == 0)
                    {
                        if (cli->islogin == 1)
                        {
                            Write(STDOUT_FILENO, "test", sizeof("test"));
                            param = strtok(NULL, " ");
                            if (param != NULL)
                            {
                                // 拿到密码
                                struct Chatroom *rm = (struct Chatroom *)malloc(sizeof(struct Chatroom));
                                rm->roomid = Roomid++;
                                rm->numbers = 1;
                                sprintf(rm->password, "%s", param);
                                // 加入到list中
                                add_to_room_list(rm);
                                // 把用户的房间切换
                                cli->roomid = rm->roomid;
                            }
                        }
                        else
                        {
                            Write(sock_fd, "You have already left!\n", strlen("You have already left!\n"));
                        }
                    }
                    // 进入聊天室
                    // join roomid roompwd
                    else if (strcmp(command, "join") == 0)
                    {
                        if (cli->islogin == 1)
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
                        else
                        {
                            Write(sock_fd, "You have already left!\n", strlen("You have already left!\n"));
                        }
                    }
                    // 离开聊天室
                    else if (strcmp(command, "leave") == 0)
                    {
                        if (cli->islogin == 1)
                        {
                            if (cli->roomid != 0)
                            {
                                send_to_all(" I have left chatroom\n", cli);
                                struct Chatroom *rm = get_room(cli->roomid);
                                rm->numbers--;
                                int tempid = cli->roomid;
                                // 切换到公屏
                                cli->roomid = 0;
                                // 判断一下聊天室的人数，如果没有人了就关闭
                                // if (is_chatroom_empty(cli->roomid))
                                if (rm->numbers == 0)
                                {
                                    delete_from_room_list(tempid);
                                }
                            }
                            else
                            {
                                Write(sock_fd, "You have already left!\n", strlen("You have already left!\n"));
                            }
                        }
                        else
                        {
                            Write(sock_fd, need_login, strlen(need_login));
                        }
                    }
                    // 注册
                    // register name password
                    else if (strcmp(command, "register") == 0)
                    {
                        char name[20], pwd[20];
                        char sql[100];
                        param = strtok(NULL, " "); // param是用户名
                        strcpy(name, param);
                        if (param != NULL)
                        {
                            param = strtok(NULL, " "); // param是密码
                            strcpy(pwd, param);
                            // 然后将用户插入到数据库中去
                            int no = query(conn, "select * from user") + 1;
                            sprintf(sql, "insert into user values(%d,'%s','%s')", no, name, pwd);
                            query(conn, sql);
                            Write(sock_fd, "Create Account Successfully!\n", strlen("Create Account Successfully"));
                        }
                    }
                    // 登陆
                    else if (strcmp(command, "login") == 0)
                    {
                        char name[20], pwd[20], sql[100];
                        param = strtok(NULL, " "); // param是用户名
                        strcpy(name, param);
                        if (param != NULL)
                        {
                            param = strtok(NULL, " "); // param是密码
                            strcpy(pwd, param);
                            // 判断是否用户名和密码正确
                            sprintf(sql, "select * from user where name='%s' and password='%s'", name, pwd);
                            int ok = query(conn, sql);
                            if (ok == 1)
                            {
                                Write(sock_fd, "login successfully!\n", strlen("login successfully!\n"));
                                // 然后干其他的事情
                                strcpy(cli->name, name);
                                strcpy(cli->password, pwd);
                                cli->islogin = 1;
                            }
                        }
                    }
                    // 修改密码
                    else if (strcmp(command, "chgpwd") == 0)
                    {
                        if(cli->islogin == 1){
                            param = strtok(NULL, " ");  // param是旧密码
                            char oldpwd[20],newpwd[20],sql[100];
                            char test[20];
                            if (param != NULL){
                                strcpy(oldpwd, param);
                                // 如果旧密码对了
                                if(strcmp(oldpwd,cli->password) == 0){
                                    param = strtok(NULL, " ");  // param是新密码
                                    strcpy(newpwd,param);
                                    sprintf(sql,"update user set password='%s' where name='%s'",newpwd,cli->name);
                                    int ok = mysql_query(conn,sql);
                                    if (ok != 0){
                                        Write(sock_fd,"Change password fail!\n",strlen("Change password fail!\n"));
                                    }else{
                                        Write(sock_fd,"Change password successfully!\n",strlen("Change password successfully!\n"));
                                        
                                        strcpy(cli->password,newpwd);
                                        
                                    }
                                    
                                }
                                else{
                                    Write(sock_fd,"Wrong old password!\n",strlen("Wrong old password!\n"));
                                }
                            }
                        }else{
                            Write(sock_fd, need_login, strlen(need_login));
                        } 
                    }
                    else if (strcmp(command, "chgname") == 0)
                    {
                        if(cli->islogin == 1){
                            param = strtok(NULL," ");   // param是新name
                            char newname[32];
                            if(param != NULL){
                                strcpy(newname,param);
                                char sql[100];
                                sprintf(sql,"update user set name='%s' where name='%s'",newname,cli->name);
                                int ok = mysql_query(conn,sql);
                                    if (ok != 0){
                                        Write(sock_fd,"Change name fail!\n",strlen("Change name fail!\n"));
                                    }else{
                                        Write(sock_fd,"Change name successfully!\n",strlen("Change name successfully!\n"));
                                        
                                        strcpy(cli->name,newname);
                                        
                                    }
                            }
                            

                        }
                    }
                    // 登出
                    else if (strcmp(command, "logout") == 0)
                    {
                        Write(sock_fd, "logout successfully!\n", strlen("logout successfully!\n"));
                        cli->islogin = 0;
                    }
                    // 错误指令
                    else{
                        Write(sock_fd, hint, strlen(hint));
                    }
                }
            }
        }
    }

    return 0;
}