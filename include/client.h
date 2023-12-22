#ifndef CLIENT_H_
#define CLIENT_H_

#include <cstddef>
#include <iostream>
#include <string>

#define size_int_t int32_t

class Client{
public:
    Client(const std::string ip); // 指定客户端的ip
    ~Client();
    size_int_t createChannel(const size_int_t port); // 创建socket套接字IP是定的，需要传端口
    size_int_t ConnAndDestory(const size_int_t clientPort, const std::string serverIp, const size_int_t serverPort, const std::string data); // 客户端请求建立连接或者断开连接，返回0表示第一次连接，返回1表示已经连接过了
    void sendData(size_int_t socket_d, std::string data); // 发送数据
    size_int_t acceptTask(const size_int_t port, const char* taskPathName, const char* pluginPathName, const std::string daemonIp, const size_int_t daemonPort); // 客户端接受任务进行测试
    size_int_t createDaemon(const size_int_t daemonPort, const std::string serverIp, const size_int_t serverPort); // 创建守护进程用于检测客户端进程的状态
    void registerSignal();
    void setIsConnect(int state);
    int getIsConnect();
private:
    std::string address; // 客户端的IP地址
    int isConnect; // 用于标记是否已经和客户端连接过了
public:
    int client_port;
};
extern size_int_t sigFlag; // 信号变量，信号传递时修改变量，用于终止线程
void sig_int(size_int_t); // 捕捉信号SIGINT之后，执行自定义的动作
void sig_quit(size_int_t); // 捕捉信号SIGQUIT之后，执行自定义的动作

// size_int_t createConnThread(Client &client, const size_int_t clientPort, const std::string serverIp, const size_int_t serverPort, const std::string data); // 创建接收客户端连接以及撤销线程, 返回0代表线程创建成功，1代表线程创建失败

size_int_t createDaemonThread(Client &client, int port);

size_int_t createAcceptTasksThread(Client &client, int port);

#endif