#include <iostream>
#include <strings.h>
#include "JKRCompression.h"
#include "JKRArchive.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    char* filePath = argv[1];
    char* outputPath;
    bool fastComp = false;

    for (s32 i = 1; i < argc; i++) {
        if (!strcasecmp(argv[i], "-u") || !strcasecmp(argv[i], "--unpack")) {
            printf("Checking for compression!\n");
            u8* pData = JKRCompression::decode(filePath, &bufferSize);
            JKRArchive* archive;

            if (!pData) {
                archive = new JKRArchive(filePath);       
            }
            else {
                archive = new JKRArchive(pData, bufferSize);
            }
            
            std::string path = filePath;
            u32 lastSlashIdx = path.rfind('\\');
            std::string dir = path.substr(0, lastSlashIdx);

            archive->unpack(dir);
            free(archive);
        }
        else if (!strcasecmp(argv[i], "-p") || !strcasecmp(argv[i], "--pack")) {
            printf("Packing!\n");
    
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

            JKRArchive* archive = new JKRArchive();
            archive->importFromFolder(filePath);

            printf("Compressing!\n");
            //JKRCompression::encode(filePath, compType, fast);
            free(archive);
        }
    }

    return 0;
}