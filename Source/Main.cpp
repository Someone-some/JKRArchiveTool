#include <iostream>
#include <strings.h>
#include "BinaryReader.h"
#include "Yaz0.h"
#include "Util.h"
#include "RARC.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    char*pFilePath = argv[1];

    for (s32 i = 1; i < argc; i++) {
        if (!strcasecmp("--decomp", argv[i]) || !strcasecmp("-d", argv[i])) {
            if (Yaz0::check(pFilePath))
                Yaz0::decomp(pFilePath, &bufferSize, true);
            RARC* archive = new RARC(pFilePath);
        }
        else if (!strcasecmp("--comp", argv[i]) || !strcasecmp("-c", argv[i])) {
            bool fastComp = false;
            for (s32 y = 0; y < argc; y++) {
                if (!strcasecmp("--fast", argv[y]) || !strcasecmp("-f", argv[y]))
                    fastComp = true;
            }

            if (fastComp)
                Yaz0::fastComp(pFilePath);
            else
                Yaz0::comp(pFilePath);
        }
    }
    printf("Done!\n");
    return 0;
}