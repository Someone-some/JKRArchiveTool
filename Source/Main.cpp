#include <iostream>
#include <strings.h>
#include "BinaryReader.h"
#include "Yaz0.h"
#include "Util.h"
#include "RARC.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    char* filePath = argv[1];
    char* outputPath;
    bool fastComp = false;
    std::cout << filePath << std::endl;

    // if (!strcasecmp("--help", argv[1]) || !strcasecmp("-h", argv[1])) {
    //     printf("[FilePath] [-d/--decomp] [-c/--comp] [-u/--unpack] [-f/--fast]\n[-f] increases the speed of compression, the downside is that the files are bigger\n");
    //     return 0;
    // }

    // for (s32 i = 1; i < argc; i++) {
    //     if (!strcasecmp("--decomp", argv[i]) || !strcasecmp("-d", argv[i])) {
    //         if (Yaz0::check(filePath)) {
    //             printf("Decompressing: %s\n", filePath);
    //             Yaz0::decomp(filePath, &bufferSize, true);
    //         }
    //     }
    //     else if (!strcasecmp("--comp", argv[i]) || !strcasecmp("-c", argv[i])) {
    //         fastComp = false;
    //         for (s32 y = 0; y < argc; y++) {
    //             if (!strcasecmp("--fast", argv[y]) || !strcasecmp("-f", argv[y]))
    //                 fastComp = true;
    //         }

    //         printf("Compressing: %s\n", filePath);
    //         if (fastComp)
    //             Yaz0::fastComp(filePath);
    //         else
    //             Yaz0::comp(filePath);
    //     }
    //     else if (!strcasecmp("--unpack", argv[i]) || !strcasecmp("-u", argv[i])) {
    //         printf("Unpacking: %s\n", filePath);
    //         u8* pData;
    //         RARC* archive;
    //         if (Yaz0::check(filePath)) {
    //             pData = Yaz0::decomp(filePath, &bufferSize, false);
    //             archive = new RARC(pData, bufferSize);
    //         }
    //         else {
    //             archive = new RARC(filePath);
    //         }
    //         std::string path = filePath;
    //         u32 lastSlashIdx = path.rfind('\\');
    //         std::string dir = path.substr(0, lastSlashIdx);
    //         archive->exportContents(dir);
    //         archive->~RARC();
    //     }
    // }
    // printf("Complete!\n");

    return 0;
}