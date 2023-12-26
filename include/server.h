#ifndef SERVER_H_
#define SERVER_H_

#include <bits/types/sig_atomic_t.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#define size_int_t int32_t

class Server{
public:
    Server(const std::string ip); // 服务器的构造函数，用于指定服务器的ip
    ~Server(); // 服务器的析构函数，用于处理服务器销毁的时候的扫尾工作
    // size_int_t threadCreat(); // 负责服务器五个线程的创建以及销毁
    void* ConAndInvClient(Server &server,const size_int_t port); // 负责新加入的客户端的连接以及旧的客户端的撤销
    void* conn(size_int_t newsd, struct sockaddr_in &raddr);
    size_int_t distributeTasks(const std::string ip, const size_int_t port, const std::string taskPathName, const std::string pluginPathName); // 负责给客户端分发任务
    size_int_t distributeStrategy(const std::string taskPathName, const std::string pluginPathName); // 任务分发策略 返回1表明分发成功，返回0表明分发失败
    size_int_t RecvResult(const size_int_t recvResultPort); // 回收客户端执行完任务之后的结果回收
    size_int_t getAllStates(); // 负责和客户端的守护进程通信，获取所有的客户端状态
    size_int_t showTasksStates(); // 展示所有任务的状态
    size_int_t getStates(const size_int_t localport); // 接受客户端主动发送的状态更新请求
    size_int_t createChannel(const size_int_t port); // 创建socket套接字IP是定的，需要传端口
    void sendData(size_int_t socket_d, std::string data); // 发送字符数据
    void sendBinaryData(size_int_t socket_d, const char data[BUFSIZ], size_int_t len); // 传输二进制数据
    void registerSignal(); // 服务器注册信号行为
    void tasksInit(); // 任务初始化
private:
    typedef struct {
        std::string name; // 客户端的名字 用客户端的ip代替
        size_int_t linkState;    // 客户端的连接状态，1表示处于连接状态，0表示断开连接
        size_int_t linkCount;    // 客户端连接次数，客户端可能会连接-断开-连接，多次连接的状态，该参数记录了客户端连接的次数
        size_int_t isBusy;       // 该参数表示客户端是否是在执行测试任务，如果客户端处于执行测试任务状态中，则是busy状态，
                          // 是1，否则处于unbusy状态是0
        size_int_t clientPort; // 客户端的端口
    } ClientStateNode;
    std::map<std::string, ClientStateNode> clientStateMap; // 服务器端用于维护客户端的状态信息的map，是通过客户端的名字来检索的，
                                                           // 客户端的名字可以用客户端的IP来代替
    std::string address; // 服务器需要绑定的IP
    typedef struct{
        int isAssign = 0; // 任务是否发放，0表示还没有发放， 1表示以及发放
        int isFinished = 0; // 任务是否完成，0表示任务还没有完成，1表示任务完成
        std::string pluginName; // 该任务需要的插件的名字
        std::string clientName; // 任务分派之后的客户端
    } TASK;
    std::vector<std::string> tasksLists;
    std::map<std::string, TASK> taskMap;
};
extern volatile sig_atomic_t sigFlag; // 信号变量，信号传递时修改变量，用于终止线程
void sig_int(size_int_t); // 捕捉信号SIGINT之后，执行自定义的动作
void sig_quit(size_int_t); // 捕捉信号SIGQUIT之后，执行自定义的动作

size_int_t createConnThread(Server &server, const size_int_t port); // 创建接收客户端连接以及撤销线程, 返回0代表线程创建成功，1代表线程创建失败

size_int_t createGetStateThread(Server &server); // 接受客户端发送的更新状态请求


#endif