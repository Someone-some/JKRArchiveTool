#include <strings.h>
#include "JKRCompression.h"
#include "JKRArchive.h"
#include "Util.h"

int main(int argc, char*argv[]) {  
    u32 bufferSize;
    bool fastComp = false;

    if (!strcasecmp(argv[1], "-h") || !strcasecmp(argv[1], "--help") || argc == 1) {
        printf("usage: JKRArchiveTools.exe [-OPTIONS]\n");
        printf("<Required>\n");
        printf("-u/--unpack [*.arc] # unpacks the given archive\n");
        printf("-p/--pack [*]       # packs the given folder into an archive\n");
        printf("\n<Packing options>\n");
        printf("-o/--out [*.arc]    # (optional) the ouput file name\n");
        printf("-szs                # compresses the output archive with szs compression\n");
        printf("-szp                # compresses the output archive with szp compression\n");
        printf("-f/--fast           # increases compression speed at the expense of file size\n");
        printf("-Os                 # attempts to decrease archive size by removing duplicate strings\n");
        printf("<File attributes>\n");
        printf("MRAM                # (default) preload file to main RAM\n");
        printf("ARAM                # (Gamecube only) preload file to auxiliary RAM\n");
        printf("DVD                 # load file from DVD\n");
        printf("<Other>\n");
        printf("-h/--help           # show usage\n");
        return 0;
    }

    for (s32 i = 0; i < argc; i++) {
        if (!strcasecmp(argv[i], "-u") || !strcasecmp(argv[i], "--unpack")) {
            std::string filePath = argv[i + 1];

            if (!File::FileExists(filePath)) {
                printf("File isn't exist!\n");
                return 1;
            }
            
            printf("Checking for compression!\n");
            u8* pData = JKRCompression::decode(filePath, &bufferSize);
            JKRArchive* archive;

            if (!pData)
                archive = new JKRArchive(filePath);       
            else 
                archive = new JKRArchive(pData, bufferSize);
            
            std::string path = filePath;
            u32 lastSlashIdx = path.rfind('\\');
            std::string dir = path.substr(0, lastSlashIdx);
            dir = dir.substr(0, dir.rfind("."));
            archive->unpack(dir);
            delete archive;
            delete pData;
        }
        else if (!strcasecmp(argv[i], "-p") || !strcasecmp(argv[i], "--pack")) {
            std::string filePath = argv[i + 1];
            printf("Packing!\n");
    
            JKRCompressionType compType = JKRCompressionType_NONE;
            JKRFileAttr attr = JKRFileAttr_FILE;
            bool fast = false;
            bool optimise = false;
            std::string outputPath = filePath + ".arc";

            for (s32 i = 1; i < argc; i++) {
                if (!strcasecmp(argv[i], "-szs")) 
                    compType = JKRCompressionType_SZS;
                else if (!strcasecmp(argv[i], "-szp")) 
                    compType = JKRCompressionType_SZP;
                
                if (!strcasecmp(argv[i], "-f") || !strcasecmp(argv[i], "--fast"))
                    fast = true;

                if (!strcasecmp(argv[i], "-Os"))
                    optimise = true;

                if (!strcasecmp(argv[i], "-o") || !strcasecmp(argv[i], "--out")) {
                    u32 lastSlashIdx = outputPath.rfind('\\');
                    std::string name = outputPath.substr(lastSlashIdx + 1);
                    outputPath.erase(outputPath.begin() + lastSlashIdx + 1, outputPath.end());
                    outputPath.append(argv[i + 1]);
                }

                if (!strcasecmp(argv[i], "-a") || !strcasecmp(argv[i], "--attr")) {
                    for (s32 y = i; y < i + 3; y++) {
                        if (!strcasecmp(argv[y], "mram"))
                            attr = (JKRFileAttr)(attr | JKRFileAttr_LOAD_TO_MRAM);
                        else if (!strcasecmp(argv[y], "aram"))
                            attr = (JKRFileAttr)(attr | JKRFileAttr_LOAD_TO_ARAM);
                        else if (!strcasecmp(argv[y], "dvd"))
                            attr = (JKRFileAttr)(attr | JKRFileAttr_LOAD_FROM_DVD);

                        if (!strcasecmp(argv[y], "szs")) {
                            attr = (JKRFileAttr)(attr | JKRFileAttr_COMPRESSED);
                            attr = (JKRFileAttr)(attr | JKRFileAttr_USE_SZS);
                        }
                    }
                }
            }

            if (attr = JKRFileAttr_FILE)
                attr = (JKRFileAttr)(attr | JKRFileAttr_LOAD_TO_MRAM);

            JKRArchive* archive = new JKRArchive();
            archive->importFromFolder(filePath, attr);
            archive->save(outputPath, optimise);
            delete archive;

            if (compType != JKRCompressionType_NONE) {
                printf("Compressing!\n");
                JKRCompression::encode(outputPath, compType, fast);
            }
        }
    }
    printf("Complete!");

    return 0;
}