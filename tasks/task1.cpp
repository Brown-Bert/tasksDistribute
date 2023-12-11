#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <dlfcn.h>
#include <dirent.h>
#include <cstring>

int main(){

    // 加载动态库
    void* handle = dlopen("./function1.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "无法打开共享库：%s\n", dlerror());
        exit(1);
    }

    typedef int (*myprint)(const char*);
    // 映射动态库中的计算行数的函数
    myprint print = (myprint)dlsym(handle, "myprint");
    if (!print) {
        fprintf(stderr,"无法获取函数指针：%s\n", dlerror());
        exit(1);
    }
    // 执行测试
    print("function1 test()\n");

    // 关闭动态库
    dlclose(handle);
    exit(0);
}