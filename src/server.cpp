#include "../include/server.h"

#include <bits/types/siginfo_t.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <system_error>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <functional>
#include <mutex>

#define IPSIZE 16
#define BUFSIZE 1024
volatile sig_atomic_t sigFlag = 0;
std::mutex mtx_map; // 对服务器的状态管理进行加锁

// 服务器的构造函数，用于指定服务器的ip以及端口
Server::Server(const std::string ip){
    address = ip;
}

Server::~Server(){


}

// 捕捉信号SIGINT之后，执行自定义的动作
void sig_int(size_int_t sig, siginfo_t *info, void *context){
    sigFlag = 1;
    std::cout << sigFlag << std::endl;
}

// 捕捉信号SIGQUIT之后，执行自定义的动作
void sig_quit(size_int_t){
    sigFlag = 1;
}

// 服务器注册信号行为
void Server::registerSignal(){
    // if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
    // signal(SIGINT, sig_int);
    // }
    // if (signal(SIGQUIT, SIG_IGN) != SIG_IGN) {
    //     signal(SIGQUIT, sig_quit);
    // }

    // 设置信号处理函数
    struct sigaction sa;
    sa.sa_sigaction = sig_int;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_restorer = NULL;
    sigaction(SIGINT, &sa, NULL);
}

// 创建socket套接字IP是定的，需要传端口
size_int_t Server::createChannel(const size_int_t port){
    struct sockaddr_in laddr;
    size_int_t socket_d = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_d < 0){
        perror("socket()");
        size_int_t state = 1;
        pthread_exit(&state);
    }

    size_int_t val = 1;
    if (setsockopt(socket_d, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(size_int_t)) < 0) {
        perror("setsockopt()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    if (port != 0) { // 端口不为0，才需要与本地进行绑定（服务器也需要向客户端请求，当服务器作为请求方的时候，就不需要跟本地进行绑定端口）
        laddr.sin_family = AF_INET;
        laddr.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &laddr.sin_addr.s_addr);
        if (bind(socket_d, (const struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
            perror("bind()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
    }
    return socket_d;
}

// 收到方返回的已收到
void response(size_int_t newsd){
    char respone[4];
    respone[0] = 'G';
    respone[1] = 'E';
    respone[2] = 'T';
    respone[3] = '\0';
    if (write(newsd, respone, 4) < 0) {
    perror("write()");
    }
}

void* Server::conn(size_int_t newsd, struct sockaddr_in &raddr){
    char buf[BUFSIZE];
    char ipbuf[IPSIZE]; // 用于记录请求连接的客户端ip从二进制转成字符串
    inet_ntop(AF_INET, &raddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
    size_int_t num = read(newsd, buf, BUFSIZE);
    std::string strs(buf, num - 1);
    if (strs == "CONNECT") {
        // 表示新的客户端请求加入
        response(newsd); // 返回给客户端已收到的信息
        // 往服务器的客户端状态记录器中写入信息
        // 加锁
        mtx_map.lock();
        ClientStateNode node;
        std::string colon = ":";
        node.name = ipbuf + colon + std::to_string(ntohs(raddr.sin_port));
        node.linkState = 1;
        node.isBusy = 0;
        if (clientStateMap.find(node.name) != clientStateMap.end()) { // 请求连接的客户端之前连接过
            // std::cout << ipbuf << " 之前连接过\n" << std::endl;
            ClientStateNode temp = clientStateMap.at(node.name);
            node.linkCount = temp.linkCount + 1;
            node.clientPort = ntohs(raddr.sin_port);
            // std::cout << "SERVER from CLIENT: " << ipbuf << ":" << ntohs(raddr.sin_port) << std::endl;
        }else { // 请求连接的客户端之前没有连接过服务器
            node.linkCount = 1;
            node.clientPort = ntohs(raddr.sin_port);
        }
        clientStateMap[node.name] = node;
        // 解锁
        mtx_map.unlock();
    }
    if (strs == "DESTORY") {
        // 表示客户端请求撤出
        // 加锁
        response(newsd); // 返回给客户端已收到的信息
        mtx_map.lock();
        std::string colon = ":";
        std::string name = ipbuf + colon + std::to_string(ntohs(raddr.sin_port));
        ClientStateNode node = clientStateMap.at(name);
        node.linkState = 0;
        clientStateMap[node.name] = node;
        // 解锁
        mtx_map.unlock();
    }
    pthread_exit(0);
}

// 负责新加入的客户端的连接以及旧的客户端的撤销
void* Server::ConAndInvClient(Server &server, const size_int_t port){
    struct sockaddr_in raddr; // 记录客户端的连接信息，例如客户端的ip以及端口
    socklen_t len = sizeof(raddr);
    size_int_t newsd;
    size_int_t socket_d = createChannel(port);
    if (listen(socket_d, 50) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
        perror("listen()");
        size_int_t state = 1;
        pthread_exit(&state);
    }

    while (true) { // 一直处于循环状态，用于接收来自客户端的连接请求，然后把和客户端的通信任务交给子进程
        newsd = accept(socket_d, (struct sockaddr *)&raddr, &len);
        // if (sigFlag == 1) {
        //     perror("accept()"); // 打断了accept系统调用，accept会报错，检验是否报错
        //     close(socket_d);
        //     break;
        // }
        if (newsd < 0) {
            perror("accept()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
        
        /**
            创建子线程执行，不能创建子进程，因为父子进程所处的空间不同
        */
        std::thread t(&Server::conn, &server, std::ref(newsd), std::ref(raddr));
        t.detach();
    }
    size_int_t state = 0;
    pthread_exit(&state);
}

// 负责给客户端分发任务
size_int_t Server::distributeTasks(const std::string ip, const size_int_t port, const std::string taskPathName, const std::string pluginPathName){
    struct sockaddr_in raddr; // 连接远程的地址
    // 创建任务分发通道
    size_int_t socket_d = createChannel(0); // 向端口号为port的客户端发送任务分发请求
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(),&raddr.sin_addr.s_addr);
    if (connect(socket_d, (const struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
        perror("connect()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    // 打开任务文件以及插件文件，把数据传输给客户端
    std::string name1("../tasks/run/");
    name1 += taskPathName;
    // std::string last(".cpp");
    // name1 += last;
    size_int_t task_fd = open(name1.c_str(), O_RDONLY); // 打开文件，写入任务信息
    if (task_fd < 0) {
      perror("task-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    std::string name2("../tasks/plugins/");
    name2 += pluginPathName;
    size_int_t plugin_fd = open(name2.c_str(), O_RDONLY); // 打开文件，写入插件信息
    if (plugin_fd < 0) {
      perror("plugin-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    // std::string name3("../tasks/");
    // name3 += pluginPathName.substr(0, pluginPathName.find_last_of("."));
    // name3 += ".h";
    // // std::cout << "name3 = " << name3 << std::endl;
    // size_int_t h_fd = open(name3.c_str(), O_RDONLY); // 打开文件，写入插件信息
    // if (h_fd < 0) {
    //   perror("h-open()");
    //   size_int_t state = 1;
    //   pthread_exit(&state);
    // }
    char buf[BUFSIZE];
    // 发送任务信息
    sendBinaryData(socket_d, "TASK", 4);
    sendBinaryData(socket_d, taskPathName.c_str(), taskPathName.size());
    int num = read(task_fd, buf, BUFSIZE - 1);
    while (num) {
        // sendData(socket_d, data);
        sendBinaryData(socket_d, buf, num);
        num = read(task_fd, buf, BUFSIZE - 1);
        // std::string data(buf, 0, num);
    }
    // 发送插件信息
    sendBinaryData(socket_d, "PLUGIN", 6);
    sendBinaryData(socket_d, pluginPathName.c_str(), pluginPathName.size());
    num = read(plugin_fd, buf, BUFSIZE - 1);
    while (num) {
        // sendData(socket_d, data1);
        sendBinaryData(socket_d, buf, num);
        num = read(plugin_fd, buf, BUFSIZE - 1);
        // std::string data1(buf, 0, num);
    }
    // sendData(socket_d, "H");
    // num = read(h_fd, buf, BUFSIZE);
    // std::string data2(buf, 0, num);
    // while (num) {
    //     sendData(socket_d, data2);
    //     num = read(h_fd, buf, BUFSIZE);
    //     std::string data2(buf, 0, num);
    // }
    taskMap[taskPathName].isAssign = 1;
    close(socket_d);
    return 0;
}

// 任务分发策略
size_int_t Server::distributeStrategy(const std::string taskPathName, const std::string pluginPathName){
    // 遍历客户计数器找到一个在线而且没有在执行测试任务的客户端
    size_int_t flag = 0; // 标志用于判断是不是找到了一个客户端，找到了flag是1，没有找到是0
    for (auto it : clientStateMap) {
        if (it.second.linkState == 1 and it.second.isBusy ==0) {
            // 找到了一个符合条件的客户端
            flag = 1;
            // 给该客户端发送任务具体信息以及插件信息
            std::string ip = it.first.substr(0, it.first.find_last_of(":"));
            std::cout << ip << " : " << it.second.clientPort << std::endl;
            distributeTasks(ip, it.second.clientPort, taskPathName, pluginPathName);
        }
    }
    if (flag == 0) {
        // 没有找到一个满足条件的客户端
        return 0;
    }
    return 1;
}

// 回收客户端执行完任务之后的结果回收
size_int_t Server::RecvResult(const size_int_t recvResultPort){
    struct sockaddr_in raddr; // 记录客户端的连接信息，例如客户端的ip以及端口
    socklen_t len = sizeof(raddr);
    size_int_t newsd;
    pid_t pid;
    char buf[BUFSIZE];
    char ipbuf[IPSIZE]; // 用于记录连接的客户端ip从二进制转成字符串
    size_int_t socket_d = createChannel(recvResultPort);
    if (listen(socket_d, 50) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
        perror("listen()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    while (true) {
        newsd = accept(socket_d, (struct sockaddr *)&raddr, &len);
        if (sigFlag == 1) {
            perror("accept()"); // 打断了accept系统调用，accept会报错，检验是否报错
            close(socket_d);
            break;
        }
        if (newsd < 0) {
            perror("accept()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
        inet_ntop(AF_INET, &raddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
        std::cout << "SERVER from CLIENT: " << ipbuf << ":" << ntohs(raddr.sin_port);
        pid = fork();
        if (pid < 0) {
            perror("fork()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
        if (pid == 0){
            // 子进程
            close(socket_d);
            size_int_t num = read(newsd, buf, BUFSIZE);
            std::string strs(buf, num - 1);
            if (num == 0) break;
            if (strs == "FINISHED") {
                // 表名该客户端已经完成测试
                ClientStateNode node = clientStateMap.at(ipbuf);
                node.isBusy = 0;
                clientStateMap[ipbuf] = node;
            }else {
                // 其他操作
            }
            exit(0);
        }
        close(newsd);   
    }
    return 0;
}

// 展示所有客户端的状态
size_int_t Server::getAllStates(){
    std::cout << "客户端\t\t" << "连接状态\t" << "连接次数\t" << "是否活跃(0: 不活跃, 1: 活跃)\n";
    for (auto it = clientStateMap.begin(); it != clientStateMap.end(); it++){
        std::cout << it->first << "\t" << it->second.linkState << "\t\t" << it->second.linkCount<< "\t\t" << it->second.isBusy << std::endl;
    }
    return 0;
}

// 展示所有任务的状态
size_int_t Server::showTasksStates(){
    std::cout << "任务\t" << "分配状态(0:未分配,1:已分配)\t完成状态(0:未完成,1:已完成)\t插件名" << std::endl;
    for (auto it : taskMap) {
        std::cout << it.first << "\t\t" << it.second.isAssign << "\t\t\t\t" << it.second.isFinished << "\t\t\t" << it.second.pluginName << std::endl;
    }
    return 0;
}

// 接受客户端发送的更新状态请求
size_int_t createGetStateThread(Server &server){
    try {
        std::thread t(&Server::getStates, &server, 5555);
        // t.join();
        t.detach();
        return 0;
    }catch (const std::system_error &e) {
        std::cerr << "Failed to create getstate thread: " << e.what() << std::endl;
        return 1;
    }
}

// 接受客户端主动发送的状态更新请求
size_int_t Server::getStates(const size_int_t localport){
    struct sockaddr_in raddr; // 记录客户端的连接信息，例如客户端的ip以及端口
    socklen_t len = sizeof(raddr);
    size_int_t newsd;
    pid_t pid;
    char buf[BUFSIZE];
    char ipbuf[IPSIZE]; // 用于记录连接的客户端ip从二进制转成字符串
    size_int_t socket_d = createChannel(localport);
    if (listen(socket_d, 50) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
        perror("listen()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    // puts("13579");
    while (true) {
        newsd = accept(socket_d, (struct sockaddr *)&raddr, &len);
        if (sigFlag == 1) {
            perror("accept()"); // 打断了accept系统调用，accept会报错，检验是否报错
            close(socket_d);
            break;
        }
        if (newsd < 0) {
            perror("accept()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
        inet_ntop(AF_INET, &raddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
        std::cout << "SERVER from CLIENT: " << ipbuf << ":" << ntohs(raddr.sin_port);
        std::cout << "ipbuf: " << ipbuf << std::endl; 
        pid = fork();
        if (pid < 0) {
            perror("fork()");
            size_int_t state = 1;
            pthread_exit(&state);
        }
        if (pid == 0){
            // 子进程
            close(socket_d);
            size_int_t num = read(newsd, buf, BUFSIZE);
            std::string strs(buf, num - 1);
            std::string t = strs.substr(0, strs.find_first_of("+"));
            std::string name(":");
            if (num == 0) break;
            if (t == "ACTIVE") {
                // 修改客户端为活跃状态
                name = ipbuf + name + strs.substr(strs.find_first_of("+") + 1);
                std::cout << "name = " << name << std::endl;
                // ClientStateNode node = clientStateMap.at(name);
                // node.isBusy = 1;
                clientStateMap[name].isBusy = 1;
                std::cout << "node: " << clientStateMap[name].isBusy << std::endl; 
            }else if (t == "UNACTIVE"){
                // 修改客户端为不活跃状态
                name = ipbuf + name + strs.substr(strs.find_first_of("+") + 1);
                std::cout << "name = " << name << std::endl;
                ClientStateNode node = clientStateMap.at(name);
                node.isBusy = 0;
                clientStateMap[name] = node;
            }
            exit(0);
        }
        close(newsd);   
    }
    return 0;
}

// 创建接收客户端连接以及撤销线程
size_int_t createConnThread(Server &server, const size_int_t port){
    try {
        std::thread t(&Server::ConAndInvClient, &server, std::ref(server), port);
        // t.join();
        t.detach();
        return 0;
    }catch (const std::system_error &e) {
        std::cerr << "Failed to create thread, which receves the connection and destruction from client: " << e.what() << std::endl;
        return 1;
    }
}

// 传输二进制数据
void Server::sendBinaryData(size_int_t socket_d, const char data[BUFSIZE], size_int_t len){
    char buf[BUFSIZE];
    for (int i = 0; i < len; i++) {
        buf[i] = data[i];
    }
    buf[len] = '\0';
    int res = write(socket_d, buf, len + 1);
    if (res < 0) {
        perror("write()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    read(socket_d, buf, 4);
    puts(buf);
}

// 发送数据
void Server::sendData(size_int_t socket_d, std::string data){
//   int len = sizeof(data);
  char buf[BUFSIZE];
  unsigned int i;
  for (i = 0; i < data.size(); i++) {
    if ((i % (BUFSIZE - 1) == 0) && (i != 0)) {
      buf[BUFSIZE - 1] = '\0';
      int res = write(socket_d, buf, BUFSIZE);
      if (res < 0) {
        perror("write()");
        size_int_t state = 1;
        pthread_exit(&state);
      }
      read(socket_d, buf, 4);
      puts(buf);
    }
    int index = i % (BUFSIZE - 1);
    buf[index] = data[i];
  }
  buf[i] = '\0';
  int res = write(socket_d, buf, i + 1);
  if (res < 0) {
    perror("write()");
    size_int_t state = 1;
    pthread_exit(&state);
  }
  read(socket_d, buf, 4);
  puts(buf);
}

// 任务初始化
void Server::tasksInit(){
    TASK task1, task2;
    task1.isAssign = 0;
    task1.isFinished = 0;
    task1.pluginName = "function1.so";
    task2.isAssign = 0;
    task2.isFinished = 0;
    task2.pluginName = "function2.so";
    taskMap["task1"] = task1;
    taskMap["task2"] = task2;
}