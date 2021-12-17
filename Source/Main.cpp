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
            
            std::string path = filePath;
            u32 lastSlashIdx = path.rfind('\\');
            std::string dir = path.substr(0, lastSlashIdx);

            printf("Unpacking!\n"); 
            pArchive->unpack(dir);
        }
        else if (!strcasecmp(argv[i], "-p") || !strcasecmp(argv[i], "--pack")) {
            JKRCompressionType compType;
            bool fast = false;

            for (s32 i = 1; i < argc; i++) {
                if (!strcasecmp(argv[i], "--szs")) 
                    compType = JKRCompressionType_SZS;
                else if (!strcasecmp(argv[i], "--szp")) 
                    compType = JKRCompressionType_SZP;
                
                if (!strcasecmp(argv[i], "-f") || !strcasecmp(argv[i], "--fast"))
                    fast = true;
            }

            // do packing stuff

            JKRCompression::encode(filePath, compType, fast);
        }
    }
    printf("Done!\n");
    return 0;
}