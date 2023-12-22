#include <cstdlib>
#include <string>
#include "./function1.h"


extern "C" void myprint(std::string strs){
    std::cout << "\nfunction1: " << strs << std::endl;
}