#include "Yaz0.h"
#include "BinaryWriter.h"
#include "Util.h"

namespace Yaz0 {
    bool check(const std::string &rFilePath) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        std::string magic = reader->readString(0x4);
        reader->~BinaryReader();

        return magic == "Yaz0";
    }
    
    u8* decomp(const std::string &rFilePath, u32 *bufferSize, bool writeToFile) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        if (reader->readString(0x4) != "Yaz0") {
            printf("Invalid identifier! Expected Yaz0\n");
            return nullptr;
        }

        u32 decompSize = reader->readU32();
        u8* dst = new u8[decompSize];
        *bufferSize = decompSize;
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

        if (writeToFile)
            File::writeAllBytes(rFilePath, dst, decompSize);
        return dst;
    }

    void comp(const std::string &rFilePath) {
        u32 srcSize;
        u8* src = File::readAllBytes(rFilePath, &srcSize);
        BinaryWriter* writer = new BinaryWriter(rFilePath, EndianSelect::Big);
        writer->writeString("Yaz0");
        writer->writeU32(srcSize);
        writer->writePadding(0x0, 8);
        u8 dst[24];
        Ret ret = {0, 0};
        s32 dstSize = 0;
        s32 percent = 0;

        u32 validBitCount = 0;
        u8 currCodeByte = 0;
        while(ret.mSrcPos < srcSize) {
            u32 numBytes;
            u32 matchPos;
            u32 srcPosBak;

            numBytes = compAdvanced(src, srcSize, ret.mSrcPos, &matchPos);
            if (numBytes < 3) {
                dst[ret.mDstPos] = src[ret.mSrcPos];
                ret.mDstPos++;
                ret.mSrcPos++;
                currCodeByte |= (0x80 >> validBitCount);
            }
            else {
                u32 dist = ret.mSrcPos - matchPos - 1; 
                u8 byte1, byte2, byte3;

                if (numBytes >= 0x12) {
                    byte1 = 0 | (dist >> 8);
                    byte2 = dist & 0xff;
                    dst[ret.mDstPos++] = byte1;
                    dst[ret.mDstPos++] = byte2;

                    if (numBytes > 0xff + 0x12)
                        numBytes = 0xff + 0x12;
                        
                    byte3 = numBytes - 0x12;
                    dst[ret.mDstPos++] = byte3;
                } 
                else {
                    byte1 = ((numBytes - 2) << 4) | (dist >> 8);
                    byte2 = dist & 0xff;
                    dst[ret.mDstPos++] = byte1;
                    dst[ret.mDstPos++] = byte2; 
                }
                ret.mSrcPos += numBytes;
            }
            validBitCount++;

            if (validBitCount == 8) {
                writer->writeU8(currCodeByte);

                writer->writeBytes(dst, ret.mDstPos);
                dstSize += ret.mDstPos + 1;

                currCodeByte = 0;
                validBitCount = 0;
                ret.mDstPos = 0;
            }
        }
        if (validBitCount > 0) {
            writer->writeU8(currCodeByte);
            writer->writeBytes(dst, ret.mDstPos);
            dstSize += ret.mDstPos + 1;

            currCodeByte = 0;
            validBitCount = 0;
            ret.mDstPos = 0;
        }

        writer->~BinaryWriter();
    }

    u32 compSimple(u8*src, s32 size, s32 pos, u32 *pMatchPos) {
        s32 startPos = pos - 0x1000;
        u32 byteCount = 1;
        u32 matchPos = 0;

        if (startPos < 0) {
            startPos = 0;
        }

        for (s32 i = startPos; i < pos; i++) {
            s32 y;
            for (y = 0; y < size; y++) {
                if (src[i + y] != src[y + pos]) {
                    break;
                }
            }

            if (y > byteCount) {
                byteCount = y;
                matchPos = i;
            }
        }
        *pMatchPos = matchPos;

        if (byteCount == 2) {
            byteCount = 1;
        }
        return byteCount;
    }

    u32 compAdvanced(u8* src, s32 size, s32 pos, u32 *pMatchPos)
    {
        s32 startPos = pos - 0x1000;
        u32 numBytes = 1;
        static u32 numBytes1;
        static u32 matchPos;
        static s32 prevFlag = 0;

        if (prevFlag == 1) {
            *pMatchPos = matchPos;
            prevFlag = 0;
            return numBytes1;
        }
        prevFlag = 0;
        numBytes = compSimple(src, size, pos, &matchPos);
        *pMatchPos = matchPos;

        if (numBytes >= 3) {
            numBytes1 = compSimple(src, size, pos + 1, &matchPos);
            if (numBytes1 >= numBytes+2) {
                numBytes = 1;
                prevFlag = 1;
            }
        }
        return numBytes;
    }
};