#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>

#include "../include/client.h"

Client* client_global = nullptr;
std::string PORT;
size_int_t accept_socket_d;

void func(){
    // pthread_cancel(client_global->acceptTaskT);
    std::string str("DESTORY+");
    str += PORT;
    // std::cout << str << std::endl;
    client_global->ConnAndDestory(0, "127.0.0.1", 8888,str);
}

int main(int argc, char* argv[]){

    if (argc < 3) {
        fprintf(stderr, "参数错误\n");
        exit(1);
    }
    signal(SIGINT, sig_int); // 注册信号行为
    // 挂一个钩子函数
    atexit(func);
    /**
        构造客户端的工作流程
    */
    Client client("127.0.0.1");
    client_global = &client;
    PORT = argv[1];
    client.setIsConnect(0);
    client.client_port = std::stoi(argv[1]);
    int choice;
    int logout = 0;
    while (true) {
        std::cout << "1、请求连接\n2、退出程序\n请输入选择: ";
        std::cin >> choice;
        if (choice == 1){ 
            // 连接
            if ( client.getIsConnect()== 1) {
                // 表明已经连接过了
                std::cout << "已经连接过了" << std::endl;
            } else {
                client.ConnAndDestory(std::stoi(argv[1]), "127.0.0.1", 8888,"CONNECT");
                std::cout << "连接成功" << std::endl;
                client.setIsConnect(1);
                // puts("123");
                // 开启守护线程
                createDaemonThread(client, atoi(argv[1]), atoi(argv[2]));
                std::cout << "守护线程开启\n";
                // 开启任务接受线程
                // puts("456");
                createAcceptTasksThread(client, atoi(argv[1]), atoi(argv[2]));
                std::cout << "等待任务。。。\n";
            }
        }else if (choice == 2) {
                // 退出程序
                // std::cout << client.getIsConnect() << std::endl;
                if (client.getIsConnect() != 0) {
                    raise(SIGINT);
                    close(client_global->acceptTaskSocketD);
                }
                logout = 1;
        }else{
                std::cout << "输入错误，请重新输入" << std::endl;
        }
        if (logout == 1) break;
    }
    client.setIsConnect(0);
    exit(0);
}