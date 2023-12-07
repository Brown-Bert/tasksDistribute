
#include "../include/client.h"

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <string>
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

#define BUFSIZE 1024
#define IPSIZE 16

// 客户端的构造函数，指定客户端的ip
Client::Client(const std::string ip){
    address = ip;
} 
Client::~Client(){

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
void Client::registerSignal(){
    if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
        signal(SIGINT, sig_int);
    }
    if (signal(SIGQUIT, SIG_IGN) != SIG_IGN) {
        signal(SIGQUIT, sig_quit);
    }
}

// 创建socket套接字IP是定的，需要传端口
size_int_t Client::createChannel(const size_int_t port){
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

// 客户端请求建立连接 ，向指定ip与端口发送信息
size_int_t Client::ConnAndDestory(const size_int_t clientPort, const std::string serverIp, const size_int_t serverPort, const std::string data){
    struct sockaddr_in raddr;
    size_int_t socket_d = createChannel(clientPort);
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp.c_str(), &raddr.sin_addr.s_addr);
    if (connect(socket_d, (const struct sockaddr *)&raddr, sizeof(raddr)) < 0) {
        perror("connect()");
        size_int_t state = 1;
        pthread_exit(&state);
    }
    // 向服务器发送连接请求
    sendData(socket_d, data);
    close(socket_d);
    return 0;
}

// 发送数据
void Client::sendData(size_int_t socket_d, std::string data){
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

// 客户端接受任务进行测试
size_int_t Client::acceptTask(const size_int_t port, const char* taskPathName, const char* pluginPathName, const std::string daemonIp, const size_int_t daemonPort){
  struct sockaddr_in raddr; // 记录服务器的连接信息，例如服务器的ip以及端口
  socklen_t len = sizeof(raddr);
  size_int_t newsd;
  char buf[BUFSIZE];
  char ipbuf[IPSIZE]; // 用于记录请求连接的服务端的ip从二进制转成字符串
  size_int_t socket_d = createChannel(port);
  if (listen(socket_d, 5) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
      perror("listen()");
      size_int_t state = 1;
      pthread_exit(&state);
  }
  while (true) { // 服务器可能会一直发送任务
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
    // 执行测试之前需要发送信息更新当前客户端的状态
    ConnAndDestory(0, daemonIp, daemonPort, "ACTIVE");

    inet_ntop(AF_INET, &raddr.sin_addr.s_addr, ipbuf, sizeof(ipbuf));
    std::cout << "CLIENT from SERVER: " << ipbuf << ":" << ntohs(raddr.sin_port);
    size_int_t flag = -1; // 用于标记当前收到的信息是具体的任务信息（0），还是传输的是插件信息（1）
    size_int_t task_fd = open(taskPathName, O_WRONLY); // 打开文件，写入任务信息
    if (task_fd < 0) {
      perror("task-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    size_int_t plugin_fd = open(pluginPathName, O_WRONLY); // 打开文件，写入插件信息
    if (plugin_fd < 0) {
      perror("plugin-open()");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    while (true) { // 不停地接受来自服务器的数据
      size_int_t num = read(newsd, buf, BUFSIZE);
      if (num == 0) break; // 任务具体信息以及插件信息接收完成
      std::string strs(buf, num - 1);
      if (flag == -1) {
        if (strs == "TASK") flag = 0;
        if (strs == "PLUGIN") flag = 1;
      }else if (flag == 0) {
        // 接收到的消息是具体的任务消息
        // 操作。。。。
        std::cout << strs << std::endl;
        write(task_fd, buf, num - 1);
      }else {
        // 接收到的消息是插件数据
        // 要把该数据全部写入到文件中
        write(plugin_fd, buf, num - 1);
      }
    }
    close(task_fd);
    close(plugin_fd);
    // 编译任务信息
    std::string task("g++ -o taskmain");
    task += taskPathName;
    if (std::system(task.c_str()) != 0) {
      fprintf(stderr, "编译失败\n");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    
    // 执行程序
    std::system("./taskmain");
    // 测试任务完成
    ConnAndDestory(0, daemonIp, daemonPort, "UNACTIVE");
  }
  size_int_t state = 0;
  pthread_exit(&state);
}
// 创建守护进程用于检测客户端进程的状态
size_int_t Client::createDaemon(const size_int_t daemonPort, const std::string serverIp, const size_int_t serverPort){
  struct sockaddr_in raddr; // 记录客户端的连接信息，例如客户端的ip以及端口
  socklen_t len = sizeof(raddr);
  size_int_t newsd;
  char buf[BUFSIZE];
  char ipbuf[IPSIZE]; // 用于记录请求连接的客户端的ip从二进制转成字符串
  size_int_t socket_d = createChannel(daemonPort);
  if (listen(socket_d, 5) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
      perror("listen()");
      size_int_t state = 1;
      pthread_exit(&state);
  }
  std::string logPathName; // 守护进程脱离终端，所以把打印信息输出到日志文件中
  size_int_t log_fd = open(logPathName.c_str(), O_WRONLY);
  if (log_fd < 0) {
    perror("log-open()");
    size_int_t state = 1;
    pthread_exit(&state);
  }
  while (true) {
    newsd = accept(socket_d, (struct sockaddr *)&raddr, &len); // 只需要accept一次，并不需要一直aceept，因为服务器不会一直请求连接客户端
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
    std::string logstrs = "DAEMON from CLIENT: ";
    std::string colon = ":";
    std::string wrap = "\n";
    logstrs =  ipbuf + colon + std::to_string(ntohs(raddr.sin_port)) + wrap;
    // std::cout << "DAEMON from CLIENT: " << ipbuf << ":" << ntohs(raddr.sin_port);
    size_int_t num = read(newsd, buf, BUFSIZE);
    std::string strs(buf, num - 1);
    if (strs == "ACTIVE") { // 表明客户端正在进行
      // 需要向服务器请求更新客户端的状态信息
      ConnAndDestory(0, serverIp, serverPort, "ACTIVE");
    } else if (strs == "UNACTIVE") {
      ConnAndDestory(0, serverIp, serverPort, "UNACTIVE");
    } else {
      // 客户端测试完成，然后发送结果给服务器
      ConnAndDestory(0, serverIp, serverPort, "FINISHED");
    }
  }
  size_int_t state = 0;
  pthread_exit(&state);
}