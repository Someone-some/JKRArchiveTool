#include <iostream>
#include <strings.h>
#include "BinaryReader.h"
#include "Yaz0.h"
#include "Util.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    if (!strcasecmp("-decomp", argv[2]) || !strcasecmp("-d", argv[2]))
        Yaz0::decomp(argv[1], &bufferSize, true);
    else if (!strcasecmp("-comp", argv[2]) || !strcasecmp("-c", argv[2]))
        Yaz0::comp(argv[1]);
    return 0;
}