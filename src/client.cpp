
#include "../include/client.h"

#include <asm-generic/socket.h>
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


#define BUFSIZE 1024
#define IPSIZE 16
size_int_t sigFlag = 0;
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
  // int len = sizeof(data);
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
    std::string taskName("./run/");
    std::string pluName("./plugins/");
    // taskName = taskName + taskPathName;
    // std::cout << "taskName: " << taskName << std::endl;
    // std::string last = ".cpp";
    size_int_t task_fd, plugin_fd;
    // std::string hName("./");
    // hName = hName + "function.h";
    // size_int_t h_fd = open(hName.c_str(), O_WRONLY | O_CREAT, 0644); // 打开文件，写入插件信息
    // if (h_fd < 0) {
    //   perror("h-open()");
    //   size_int_t state = 1;
    //   pthread_exit(&state);
    // }
    while (true) { // 不停地接受来自服务器的数据
      size_int_t num = read(newsd, buf, BUFSIZE);
      response(newsd);
      if (num == 0) {
        // puts("跳出");
        break; // 任务具体信息以及插件信息接收完成
      }
      std::string strs(buf, num - 1);
      if (strs == "TASK") {
        // puts("TASK");
        flag = 0;
        size_int_t num = read(newsd, buf, BUFSIZE);
        response(newsd);
        std::string strs(buf, num - 1);
        taskName += strs;
        task_fd = open(taskName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0744); // 打开文件，写入任务信息
        if (task_fd < 0) {
          perror("task-open()");
          size_int_t state = 1;
          pthread_exit(&state);
        }
        continue;
      }
      if (strs == "PLUGIN") {
        // puts("PLUGIN");
        flag = 1;
        size_int_t num = read(newsd, buf, BUFSIZE);
        // std::cout << "mum= " << num << std::endl;
        response(newsd);
        std::string strs(buf, num - 1);
        pluName += strs;
        plugin_fd = open(pluName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644); // 打开文件，写入插件信息
        if (plugin_fd < 0) {
          perror("plugin-open()");
          size_int_t state = 1;
          pthread_exit(&state);
        }
        continue;
      }
      // if (strs == "H"){
      //   flag = 2;
      //   continue;
      // }
      if (flag == 0) {
        // 接收到的消息是具体的任务消息
        // 操作。。。。
        // std::cout << strs << std::endl;
        // std::cout << "mum: " << num << std::endl;
        write(task_fd, buf, num - 1);
      }else if (flag == 1) {
        // 接收到的消息是插件数据
        // 要把该数据全部写入到文件中
        // std::cout << "mum= " << num << std::endl;
        write(plugin_fd, buf, num - 1);
      } else {
        // 接受h头文件
        // write(h_fd, buf, num - 1);
      }
    }
    // std::cout << "654321" << std::endl;
    close(task_fd);
    close(plugin_fd);
    // 编译任务信息
    // std::string task("g++ -o taskmain ");
    // task += taskName;
    // task += last;
    // task += " -ldl";
    // if (std::system("make") != 0) {
    //   fprintf(stderr, "编译失败\n");
    //   size_int_t state = 1;
    //   pthread_exit(&state);
    // }
    
    // 执行程序
    // std::cout << taskName << std::endl;
    taskName = taskName.substr(taskName.find_last_of("/"));
    std::string n("cd run && .");
    taskName = n + taskName;
    // std::system("cd run");
    if (std::system(taskName.c_str()) != 0) {
      fprintf(stderr, "运行失败\n");
      size_int_t state = 1;
      pthread_exit(&state);
    }
    // 测试任务完成
    std::system("rm ./plugins/* && rm ./run/*");
    ConnAndDestory(0, daemonIp, daemonPort, "UNACTIVE");
  }
  // std::cout << "123456" << std::endl;
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
  size_int_t socket_d = createChannel(4444);
  if (listen(socket_d, 5) < 0) { // 之所以把listen放在这里，是为了复用createChannel函数
      perror("listen()");
      size_int_t state = 1;
      pthread_exit(&state);
  }
  std::string logPathName = "log.txt"; // 守护进程脱离终端，所以把打印信息输出到日志文件中
  size_int_t log_fd = open(logPathName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (log_fd < 0) {
    perror("log-open()");
    size_int_t state = 1;
    pthread_exit(&state);
  }
  while (true) {
    newsd = accept(socket_d, (struct sockaddr *)&raddr, &len); // 只需要accept一次，并不需要一直aceept，因为服务器不会一直请求连接客户端
    // std::cout << "dem con" << std::endl;
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
    write(log_fd, logstrs.c_str(), sizeof(logstrs));
    // std::cout << "DAEMON from CLIENT: " << ipbuf << ":" << ntohs(raddr.sin_port);
    size_int_t num = read(newsd, buf, BUFSIZE);
    std::string strs(buf, num - 1);
    response(newsd);
    if (strs == "ACTIVE") { // 表明客户端正在进行
      // 需要向服务器请求更新客户端的状态信息
      puts("发送");
      std::string name("ACTIVE+");
      ConnAndDestory(0, serverIp, serverPort, (name + std::to_string(daemonPort)));
    } else if (strs == "UNACTIVE") {
      std::string name("UNACTIVE+");
      ConnAndDestory(0, serverIp, serverPort, (name + std::to_string(daemonPort)));
    } else {
      // 客户端测试完成，然后发送结果给服务器
      std::string name("FINISHED+");
      ConnAndDestory(0, serverIp, serverPort, (name + std::to_string(daemonPort)));
    }
  }
  size_int_t state = 0;
  pthread_exit(&state);
}

// // 创建接收客户端连接以及撤销线程, 返回0代表线程创建成功，1代表线程创建失败
// size_int_t createConnThread(Client &client, const size_int_t clientPort, const std::string serverIp, const size_int_t serverPort, const std::string data){
//   try {
//     std::thread t(&Client::ConnAndDestory, &client, clientPort, serverIp, serverPort, data);
//   } catch (const std::system_error &e) {
//     std::cerr << "Failed to create thread, which wants to connect and destory: " << e.what() << std::endl;
//     return 1;
//   }
// }


void Client::setIsConnect(int state){
  isConnect = state;
}
int Client::getIsConnect(){
  return isConnect;
}


size_int_t createDaemonThread(Client &client, int port){
  try {
      std::thread t(&Client::createDaemon, &client, port, "127.0.0.1", 5555);
      // t.join();
      t.detach();
      return 0;
  }catch (const std::system_error &e) {
      std::cerr << "Failed to create Daemon thread: " << e.what() << std::endl;
      return 1;
  }
}

size_int_t createAcceptTasksThread(Client &client, int port){
  try {
      std::thread t(&Client::acceptTask, &client, port, "task", "function.cpp", "127.0.0.1", 4444);
      // t.join();
      t.detach();
      return 0;
  }catch (const std::system_error &e) {
      std::cerr << "Failed to create AcceptTasks thread: " << e.what() << std::endl;
      return 1;
  }
}