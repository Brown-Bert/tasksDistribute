#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <dlfcn.h>
#include <dirent.h>
#include <cstring>
#include <unistd.h>

int main(){

    // 加载动态库
    void* handle = dlopen("../plugins/function2.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "无法打开共享库：%s\n", dlerror());
        exit(1);
    }
    sleep(20);
    typedef void (*myprint)(std::string);
    // 映射动态库中的计算行数的函数
    myprint print = (myprint)dlsym(handle, "myprint");
    if (!print) {
        fprintf(stderr,"无法获取函数指针：%s\n", dlerror());
        exit(1);
    }
    // 执行测试
    print("function2 test()\n");

    // 关闭动态库
    dlclose(handle);
    exit(0);
}