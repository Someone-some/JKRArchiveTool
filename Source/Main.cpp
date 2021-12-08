#include <iostream>
#include <strings.h>
#include "BinaryReader.h"
#include "Yaz0.h"
#include "Util.h"
#include "RARC.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    if (!strcasecmp("-decomp", argv[2]) || !strcasecmp("-d", argv[2])) {
        if (Yaz0::check(argv[1]))
            Yaz0::decomp(argv[1], &bufferSize, true);
        RARC* archive = new RARC(argv[1]);
        std::cout << archive->mFileNodes[0]->mFullName;
    }
    else if (!strcasecmp("-comp", argv[2]) || !strcasecmp("-c", argv[2]))
        Yaz0::comp(argv[1]);

    printf("Done!\n");
    return 0;
}