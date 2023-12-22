#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include "../include/server.h"


int main() {
    Server server("127.0.0.1");

    /**
        初始化 任务列表 与 插件列表
    */
    server.tasksInit();
    

    /**
        构造服务器的工作流程
    */
    // server.registerSignal(); // 注册信号行为
    int choice;
    while (true) {
        std::cout << "1、开启服务器\n2、退出程序\n请输入选择: ";
        std::cin >> choice;
        int logout = 0;
        switch (choice) {
            case 1:
                // 开启服务器接受连接请求的线程
                {
                    createConnThread(server, 8888);
                    std::cout << "服务器已开启" << std::endl;
                    // 查看所有客户端的状态以及所有任务的状态
                    int temp = logout;
                    std::string taskname;
                    std::string plugname;
                    while (true) {
                        std::cout << "1、查看所有客户端的状态\n2、查看所有任务状态\n3、任务分发\n4、退出\n请输入选择: ";
                        std::cin >> choice;
                        switch (choice) {
                            case 1:
                                server.getAllStates();
                                break;
                            case 2:
                                server.showTasksStates();
                                break;
                            case 3:
                                std::cout << "输入任务名称：";
                                std::cin >> taskname;
                                std::cout << "输入插件名称：";
                                std::cin >> plugname;
                                createGetStateThread(server);
                                server.distributeStrategy(taskname, plugname);
                                break;
                            case 4:
                                logout = 1;
                                break;
                            default:
                                std::cout << "输入错误，请重新输入" << std::endl;
                                break;
                        }
                        if (logout == 1) break;
                    }
                    logout = temp;
                    break;
                }
            case 2:
                // 退出程序
                logout = 1;
                break;
            default:
                std::cout << "输入错误，请重新输入" << std::endl;
                break;
        }
        if (logout == 1) break;
    }

    exit(0);
}