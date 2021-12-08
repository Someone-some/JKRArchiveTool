#include <iostream>
#include "BinaryReader.h"
#include "Yaz0.h"
#include "Util.h"

int main(int argc, char*argv[]) {    
    u8* buffer = Yaz0::decomp(argv[1]);
    if (buffer)
        File::writeAllBytes(argv[1], buffer);
    return 0;
}