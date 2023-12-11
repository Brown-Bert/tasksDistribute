#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../include/client.h"

int main(int argc, char* argv[]){

    if (argc < 2) {
        fprintf(stderr, "参数错误");
        exit(1);
    }

    /**
        构造客户端的工作流程
    */
    Client client("127.0.0.1");
    int choice;
    int logout = 0;
    while (true) {
        std::cout << "1、请求连接\n2、退出程序\n请输入选择: ";
        std::cin >> choice;
        if (choice == 1){ 
            // 连接
            if (client.ConnAndDestory(std::stoi(argv[1]), "127.0.0.1", 8888,"CONNECT") == 1) {
                // 表明已经连接过了
                std::cout << "已经连接过了" << std::endl;
            } else {
                std::cout << "连接成功" << std::endl;
            }
            // 开启任务接受线程
        }else if (choice == 2) {
                // 退出程序
                client.ConnAndDestory(std::stoi(argv[1]), "127.0.0.1", 8888,"DESTORY");
                logout = 1;
        }else{
                std::cout << "输入错误，请重新输入" << std::endl;
        }
        if (logout == 1) break;
    }

    exit(0);
}