#include <iostream>
#include <strings.h>
#include "BinaryReader.h"
#include "JKRCompression.h"
#include "JKRArchive.h"
#include "Util.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    char* filePath = argv[1];
    char* outputPath;
    bool fastComp = false;

    for (s32 i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "-u") || !strcasecmp(argv[i], "--unpack")) {
            printf("Decompressing!\n");
            u8* pData = JKRCompression::decode(filePath, &bufferSize);
            JKRArchive* pArchive;

            if (!pData) {
                pArchive = new JKRArchive(filePath);
            }
            else {
                pArchive = new JKRArchive(pData, bufferSize);
            }

            printf("Unpacking!\n");
        }
    }
    printf("Done!\n");
    return 0;
}