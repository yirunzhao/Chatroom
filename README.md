### 网络聊天室
- 用户管理：注册、修改密码
- 聊天室管理：用户登录、创建聊天室、设置聊天室密码
- 聊天管理：用户所发送的消息每位在线用户都可以收到，也可以单独给某位在线用户发送消息；可以查询在线用户信息
- 系统管理：提供命令帮助，让用户了解命名的格式。例如`send user1 message1`等 

### 注意
- 连接服务器之后默认是匿名用户，只能在公屏发言，需要登陆才能创建/加入聊天室，私发消息给其他用户
- 重点在于服务器server的编写，客户端只要能连接就行，可以直接使用`nc`命令连接服务端，如`nc 127.0.0.1 8888`


### 命令指南
#### 已经实现
- `send 'message'`        给所处聊天室的所有人发消息
- `sendtouser 'userid' 'messsage'`    给某个人发私信
- `help`                  查看帮助
- `list -u`               查看在线的用户
- `list -r`               查看存在的聊天室
- `createrm 'password'`   创建聊天室，密码是password
- `join 'roomid' 'roompassword'`  加入聊天室
- `leave`                 离开当前聊天室
- `login username password`     登陆
- `register username password`  注册
- `logout`          登出
