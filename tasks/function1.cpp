#include <cstdlib>
#include <string>
#include "../include/function1.h"


extern "C" void myprint(std::string strs){
    std::cout << "function1: " << strs << std::endl;
}