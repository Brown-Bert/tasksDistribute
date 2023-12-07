#ifndef SERVER_H_
#define SERVER_H_

#include <iostream>
#include <string>
#include <map>
#define size_int_t int32_t

class Server{
public:
    Server(const std::string ip); // 服务器的构造函数，用于指定服务器的ip
    ~Server(); // 服务器的析构函数，用于处理服务器销毁的时候的扫尾工作
    size_int_t threadCreat(); // 负责服务器五个线程的创建以及销毁
    size_int_t ConAndInvClient(const size_int_t port); // 负责新加入的客户端的连接以及旧的客户端的撤销
    size_int_t distributeTasks(const std::string ip, const size_int_t port, const std::string taskPathName, const std::string pluginPathName); // 负责给客户端分发任务
    size_int_t RecvResult(const size_int_t recvResultPort); // 回收客户端执行完任务之后的结果回收
    size_int_t getAllStates(); // 负责和客户端的守护进程通信，获取所有的客户端状态
    size_int_t getStates(const size_int_t localport); // 接受客户端主动发送的状态更新请求
    size_int_t createChannel(const size_int_t port); // 创建socket套接字IP是定的，需要传端口
    void sendData(size_int_t socket_d, std::string data); // 发送数据
private:
    typedef struct {
        std::string name; // 客户端的名字
        size_int_t linkState;    // 客户端的连接状态，1表示处于连接状态，0表示断开连接
        size_int_t linkCount;    // 客户端连接次数，客户端可能会连接-断开-连接，多次连接的状态，该参数记录了客户端连接的次数
        size_int_t isBusy;       // 该参数表示客户端是否是在执行测试任务，如果客户端处于执行测试任务状态中，则是busy状态，
                          // 是1，否则处于unbusy状态是0
    } ClientStateNode;
    std::map<std::string, ClientStateNode> clientStateMap; // 服务器端用于维护客户端的状态信息的map，是通过客户端的名字来检索的，
                                                           // 客户端的名字可以用客户端的IP来代替
    std::string address; // 服务器需要绑定的IP

    void registerSignal(); // 服务器注册信号行为
};
size_int_t sigFlag = 0; // 信号变量，信号传递时修改变量，用于终止线程
void sig_int(size_int_t); // 捕捉信号SIGINT之后，执行自定义的动作
void sig_quit(size_int_t); // 捕捉信号SIGQUIT之后，执行自定义的动作



#endif