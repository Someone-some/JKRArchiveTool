#include <iostream>
#include <vector>
#include <sstream>
#include "BinaryReader.h"
#include "Yaz0.h"

int main() {    
    u8* buffer = Yaz0::decomp("L:\\Coding\\lol\\AirBubble.arc");
    std::cout << buffer[0] << buffer[1] << buffer[2] << buffer[3] << std::endl;
    return 0;
}