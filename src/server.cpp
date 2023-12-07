#include "../include/server.h"

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define IPSIZE 16
#define BUFSIZE 1024

// 服务器的构造函数，用于指定服务器的ip以及端口
Server::Server(const std::string ip){
    address = ip;
}

Server::~Server(){


}

// 捕捉信号SIGINT之后，执行自定义的动作
void sig_int(size_int_t){
    sigFlag = 1;
}

// 捕捉信号SIGQUIT之后，执行自定义的动作
void sig_quit(size_int_t){
    sigFlag = 1;
}

// 服务器注册信号行为
void Server::registerSignal(){
    if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
        signal(SIGINT, sig_int);
    }
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN) {
        signal(SIGQUIT, sig_quit);
    }
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

// 负责新加入的客户端的连接以及旧的客户端的撤销
size_int_t Server::ConAndInvClient(const size_int_t port){
    struct sockaddr_in raddr; // 记录客户端的连接信息，例如客户端的ip以及端口
    socklen_t len = sizeof(raddr);
    size_int_t newsd;
    pid_t pid;
    char buf[BUFSIZE];
    char ipbuf[IPSIZE]; // 用于记录请求连接的客户端ip从二进制转成字符串
    size_int_t socket_d = createChannel(port);
    if (listen(socket_d, 50) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
        perror("listen()");
        size_int_t state = 1;
        pthread_exit(&state);
    }

    while (true) { // 一直处于循环状态，用于接收来自客户端的连接请求，然后把和客户端的通信任务交给子进程
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
        if (pid == 0) {
            close(socket_d); // 子进程中关闭用于客户端请求连接的socket字
            size_int_t num = read(newsd, buf, BUFSIZE);
            std::string strs(buf, num - 1);
            if (num == 0) break;
            if (strs == "CONNECT") {
                // 表示新的客户端请求加入
                response(newsd); // 返回给客户端已收到的信息
                // 往服务器的客户端状态记录器中写入信息
                ClientStateNode node;
                node.name = ipbuf;
                node.linkState = 1;
                node.isBusy = 0;
                if (clientStateMap.find(node.name) != clientStateMap.end()) { // 请求连接的客户端之前连接过
                    std::cout << ipbuf << " 之前连接过\n" << std::endl;
                    ClientStateNode temp = clientStateMap.at(node.name);
                    node.linkCount = temp.linkCount + 1;
                }else { // 请求连接的客户端之前没有连接过服务器
                    node.linkCount = 1;
                }
                clientStateMap[node.name] = node;
            }
            if (strs == "DESTORY") {
                // 表示客户端请求撤出
                ClientStateNode node = clientStateMap.at(ipbuf);
                node.linkState = 0;
                clientStateMap[node.name] = node;
            }
            exit(0); // 子进程中也有while循环，然后子进程只用执行通信不需要进行accept操作
        }else {
            close(newsd); // 父进程中关闭用于和客户端通信的socket字
        }
    }
    wait(NULL); // 等待所有的子进程执行完成之后，再结束主线程
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
    size_int_t task_fd = open(taskPathName.c_str(), O_RDONLY); // 打开文件，写入任务信息
    if (task_fd < 0) {
      perror("task-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    size_int_t plugin_fd = open(pluginPathName.c_str(), O_RDONLY); // 打开文件，写入插件信息
    if (plugin_fd < 0) {
      perror("plugin-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    char buf[BUFSIZE];
    // 发送任务信息
    sendData(task_fd, "TASK");
    int num = read(task_fd, buf, BUFSIZE);
    while (num) {
        sendData(task_fd, buf);
        num = read(task_fd, buf, BUFSIZE);
    }
    // 发送插件信息
    sendData(plugin_fd, "PLUGIN");
    num = read(plugin_fd, buf, BUFSIZE);
    while (num) {
        sendData(plugin_fd, buf);
        num = read(plugin_fd, buf, BUFSIZE);
    }
    close(socket_d);
    return 0;
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
    for (auto it = clientStateMap.begin(); it != clientStateMap.end(); it++){
        std::cout << "客户端\t" << "连接状态\t" << "连接次数\t" << "是否活跃\n";
        std::cout << it->first << "\t" << it->second.linkState << "\t" << it->second.linkCount<< "\t" << it->second.isBusy << std::endl;
    }
    return 0;
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
            if (strs == "ACTIVE") {
                // 修改客户端为活跃状态
                ClientStateNode node = clientStateMap.at(ipbuf);
                node.isBusy = 1;
                clientStateMap[ipbuf] = node;
            }else if (strs == "UNACTIVE"){
                // 修改客户端为不活跃状态
                ClientStateNode node = clientStateMap.at(ipbuf);
                node.isBusy = 0;
                clientStateMap[ipbuf] = node;
            }
            exit(0);
        }
        close(newsd);   
    }
    return 0;
}

// 负责服务器五个线程的创建以及销毁
size_int_t Server::threadCreat(){


}

// 发送数据
void sendData(size_int_t socket_d, std::string data){
  int len = sizeof(data);
  char buf[BUFSIZE];
  long long i;
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