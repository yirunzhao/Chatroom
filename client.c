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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define true 1
#define false 0

void print_hint()
{
    printf("*****************************************************************************************************************************************\n");
    printf("*\t\t\tThis is a chatroom made by WHU.CS.Ryan, see more details on my github: https://github.com/yirunzhao/Chatroom\t*\n");
    printf("*\t\t\tThis chatroom is the final assigment of Linux Network Programming\t\t\t\t\t\t*\n");
    printf("*\t\t\tPlease type 'zyrctrm login' to sign in or type 'zyrctrm register' tot sign up\t\t\t\t\t*\n");
    printf("*\t\t\tYou can type 'zyrctrm help' to seek for more help information\t\t\t\t\t\t\t*\n");
    printf("*****************************************************************************************************************************************\n");
}
// 列出所有在线的用户信息
void list_users()
{
    printf("列出所有在线的用户信息");
}
// 列出所有在线的聊天室
void list_rooms()
{
    printf(" 列出所有在线的聊天室");
}
// 进入聊天室
// 成功返回1，房间不存在返回-1，密码错误返回-2
int enter_room(int room_id, char *pwd)
{
    printf(" 进入聊天室");
}
// 离开当前聊天室
void leave()
{
    printf(" 离开当前聊天室");
}
// 注册用户
int user_register(char *name, char *pwd)
{
    printf(" 注册用户");
    printf("用户名：%s，密码：%s\n", name, pwd);
}
// 用户登陆
int user_login(char *name, char *pwd)
{
    printf(" 用户登陆");
    printf("用户名：%s，密码：%s\n", name, pwd);
}
// 列出所有指令
void get_help()
{
    printf("*****************************************************************************************************************************************\n");
    printf("*\t\t\tThis is a chatroom made by WHU.CS.Ryan, see more details on my github: https://github.com/yirunzhao/Chatroom\t*\n");
    printf("*\t\t\tThis chatroom is the final assigment of Linux Network Programming\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm login `username` `password`' to sign in\t\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm register `username` `password`' to sign uo\t\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm list -r' to get all online chatrooms\t\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm list -u' to get all online users\t\t\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm leave' to leave current chatroom\t\t\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm createrm `roomid` `roompwd` to create a chatroom\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm enter `roomid` `roompwd` to enter a chatroom\t\t\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm sendtouser `username` `message` to send messages to a certain user\t\t\t\t\t*\n");
    printf("*\t\t\tUse 'zyrctrm send `message` to send messages to your current chatroom\t\t\t\t\t\t*\n");
    printf("*****************************************************************************************************************************************\n");
}
// 发送消息给聊天室
void send_to_room(int room_id, char *msg)
{
    printf(" 发送消息给聊天室");
}
// 发送消息给某人
void send_to_user(char *name, char *msg)
{
    printf(" 发送消息给某人");
}
// 创建聊天室
int create_room(int room_id, char *pwd)
{
    printf(" 创建聊天室");
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
int is_login = false;
int main(void)
{
    // 进入的时候打印以下提示
    print_hint();
    // 每次的输入
    char input[80];
    // 命令，最后一个有一个回车
    char *command[10];
    int cmdi = 0;
    // 分割符号
    char delims[] = " ";
    // 分割结果
    char *result = NULL;

    while (true)
    {
        // 获取指令
        fgets(input, 80, stdin);
        printf("指令是：%s", input);
        result = strtok(input, delims);
        while (result != NULL)
        {
            command[cmdi++] = result;
            result = strtok(NULL, delims);
        }
        cmdi = 0;

        // if (strcmp(command[0], "zyrctrm") == 0)
        if (true)
        {

            // 没有登陆的时候只能注册、登陆、查看帮助
            if (is_login == false)
            {
                // 登陆
                if (strcmp(command[1], "login") == 0)
                {
                    user_login("zyr", "123");
                }
                // 注册
                if (strcmp(command[1], "register") == 0)
                {
                    user_register("zyr", "123");
                }
                // 帮助
                if (strcmp(command[1], "help\n") == 0)
                {
                    get_help();
                }
                if (strcmp(command[1], "zyr\n") == 0)
                {
                    if (is_login == true)
                        is_login = false;
                    else
                        is_login = true;
                }
            }
            // 登陆之后可以用全部功能
            else
            {
                if (strcmp(command[1], "zyr\n") == 0)
                {
                    if (is_login == true)
                        is_login = false;
                    else
                        is_login = true;
                }
                // 登陆
                if (strcmp(command[1], "login") == 0)
                {
                    user_login("zyr", "123");
                }
                // 注册
                if (strcmp(command[1], "register") == 0)
                {
                    user_register("zyr", "123");
                }
                // 帮助
                if (strcmp(command[1], "help\n") == 0)
                {
                    get_help();
                }
                // 离开
                if (strcmp(command[1], "leave\n") == 0)
                {
                    leave();
                }
                // 创建聊天室
                if (strcmp(command[1], "createrm") == 0)
                {
                    create_room(33, "123");
                }
                // 进入聊天室
                if (strcmp(command[1], "enter") == 0)
                {
                    enter_room(33, "123");
                }
                // 发送到用户
                if (strcmp(command[1], "sendtouser") == 0)
                {
                    send_to_user("zyr", "asdasd");
                }
                // 发送所有
                if (strcmp(command[1], "send") == 0)
                {
                    send_to_room(33, "asasdasdad");
                }
                // 列出信息
                if (strcmp(command[1], "list") == 0)
                {
                    if (strcmp(command[2], "-r\n") == 0)
                    {
                        list_rooms();
                    }
                    if (strcmp(command[2], "-u\n") == 0)
                    {
                        list_users();
                    }
                }
            }
        }
        else
        {
            printf("Commands must start with zyrctrm!\n");
        }
        // 需要重置数组
        bzero(command, sizeof(command));
    }
    return 0;
}