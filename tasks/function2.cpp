#include <cstdlib>
#include <string>
#include "./function2.h"


extern "C" void myprint(std::string strs){
    std::cout << "function2: " << strs << std::endl;
}