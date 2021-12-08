#include "Yaz0.h"

namespace Yaz0 {
    u8* decomp(const std::string &rFilePath) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        if (reader->readString(0x4) != "Yaz0") {
            printf("Invalid identifier! Expected Yaz0\n");
            return nullptr;
        }

        u32 decompSize = reader->readU32();
        u8* dst = new u8[decompSize];
        reader->skip(0x8);
        u32 dstPos = 0;
        u32 copySrc;
        u32 copyLen;

        while (dstPos < decompSize) {
            u8 block = reader->readU8();

            for (u32 i = 0; i < 8; i++) {
                if ((block & 0x80) != 0) {     
                    dst[dstPos] = reader->readU8();
                    dstPos++;
                }
                else {
                    u8 byte1 = reader->readU8();
                    u8 byte2 = reader->readU8();

                    copySrc = dstPos - ((byte1 & 0x0F) << 8 | byte2) -1;

                    copyLen = byte1 >> 4;
                    if (copyLen == 0) 
                        copyLen = reader->readU8() + 0x12;
                    else 
                        copyLen += 2;

                    for (u32 y = 0; y < copyLen; y++) {
                        dst[dstPos] = dst[copySrc];
                        copySrc++;
                        dstPos++;
                    }
                }
                block <<= 1;

                if (dstPos >= decompSize || reader->position() >= reader->size()) {
                    break;
                }
            }
        }     
        
        reader->~BinaryReader();
        return dst;
    }
};