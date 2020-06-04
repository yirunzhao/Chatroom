### 网络聊天室
- 用户管理：注册、修改密码
- 聊天室管理：用户登录、创建聊天室、设置聊天室密码
- 聊天管理：用户所发送的消息每位在线用户都可以收到，也可以单独给某位在线用户发送消息；可以查询在线用户信息
- 系统管理：提供命令帮助，让用户了解命名的格式。例如`send user1 message1`等 

### 命令指南
#### 已经实现
- send 'message'        给所处聊天室的所有人发消息
- sendtouser 'userid' 'messsage'    给某个人发私信
- help                  查看帮助
- list -u               查看在线的用户
- createrm 'password'   创建聊天室，密码是password
- join 'roomid' 'roompassword'  加入聊天室
- leave                 离开当前聊天室