#include "JKRCompression.h"
#include "BinaryWriter.h"
#include "Util.h"

namespace JKRCompression {
    JKRCompressionType checkCompression(const std::string &rFilePath) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        std::string magic = reader->readString(0x4);

        if (magic == "Yaz0")
            return JKRCompressionType::JKRCompressionType_SZS;
        else if (magic == "Yay0") 
            return JKRCompressionType::JKRCompressionType_SZP;
        else {
            reader->seek(0, std::ios::beg);

            if (reader->readString(0x3) == "ASR")
                return JKRCompressionType::JKRCompression_ASR;
        }
        
        reader->~BinaryReader();
        return JKRCompressionType::JKRCompressionType_NONE;
    }

    u8* decode(const std::string &rFilePath, u32 *bufferSize) {
        JKRCompressionType compType = checkCompression(rFilePath);

        switch (compType) {
            case JKRCompressionType::JKRCompressionType_NONE:
                return nullptr;
            case JKRCompressionType::JKRCompressionType_SZP:
                return decodeSZP(rFilePath, bufferSize);
            case JKRCompressionType::JKRCompressionType_SZS:
                return decodeSZS(rFilePath, bufferSize);
            case JKRCompressionType::JKRCompression_ASR:
                printf("Compression type not supported!\n");
                return nullptr;
        }

        return nullptr;
    }
    
    u8* decodeSZS(const std::string &rFilePath, u32 *bufferSize) {
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
        return dst;
    }

    u8* decodeSZP(const std::string &rFilePath, u32 *bufferSize) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        if (reader->readString(0x4) != "Yay0") {
            printf("Invalid identifier! Expected Yay0\n");
            return nullptr;
        }

        u32 decompSize = reader->readU32();
        u32 linkTableOffs = reader->readU32();
        u32 byteChunkAndCountModiferOffset = reader->readU32();

        u8* dst = new u8[decompSize];
        *bufferSize = decompSize;

        s32 maskedBitCount = 0;
        s32 curOffsInDst = 0;
        s32 curMask = 0;

        while (curOffsInDst < decompSize) {
            if (maskedBitCount == 0) {
                curMask = reader->readS32();
                maskedBitCount = 32;
            }

            if (((u32)curMask & (u32)0x80000000) == 0x80000000) {
                u64 curPos = reader->position();
                reader->seek(byteChunkAndCountModiferOffset++, std::ios::beg);
                dst[curOffsInDst++] = reader->readU8();
                reader->seek(curPos, std::ios::beg);
            }
            else {
                u64 curPos = reader->position();
                reader->seek(linkTableOffs++, std::ios::beg);
                u16 link = reader->readU16();
                linkTableOffs += 2;
                reader->seek(curPos, std::ios::beg);

                s32 offset = curOffsInDst - (link & 0xFFF);
                s32 count = link >> 0xC;

                if (count == 0) {
                    u64 curPos = reader->position();
                    reader->seek(byteChunkAndCountModiferOffset++, std::ios::beg);
                    u8 countModifer = reader->readU8();
                    reader->seek(curPos, std::ios::beg);
                    count += countModifer + 0x12;
                }
                else {
                    count += 2;
                }

                s32 blockCopy = offset;

                for (s32 i = 0; i < count; i++) {
                    dst[curOffsInDst++] = dst[blockCopy++ -1];
                }
            }

            curMask <<= 1;
            maskedBitCount--;
        }

        reader->~BinaryReader();
        return dst;
    }

    void encodeSZS(const std::string &rFilePath) {
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

            numBytes = encodeAdvancedSZS(src, srcSize, ret.mSrcPos, &matchPos);
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

    // This is faster, but the files it produces are larger
    void fastEncodeSZS(const std::string &rFilePath) {
        u32 srcSize;
        u8* src = File::readAllBytes(rFilePath, &srcSize);
        u32 pos = 0;
        u8* dst = new u8[srcSize + srcSize / 8 + 0x10];

        dst[pos++] = 'Y';
        dst[pos++] = 'a';
        dst[pos++] = 'z';
        dst[pos++] = '0';
        dst[pos++] = ((srcSize >> 24) & 0xFF);
        dst[pos++] = ((srcSize >> 16) & 0xFF);
        dst[pos++] = ((srcSize >> 8) & 0xFF);
        dst[pos++] = ((srcSize >> 0) & 0xFF);

        for (s32 i = 0; i < 8; i++) dst[pos++] = 0;
        s32 length = srcSize;
        s32 dstOffs = 16;
        s32 offs = 0;

        while (true) {
            s32 headerOffs = dstOffs++;
            pos++;
            u8 header = 0;
            for (s32 i = 0; i < 8; i++) {
                s32 comp = 0;
                s32 back = 1;
                s32 nr = 2;
                {
                    u8* ptr = src - 1;
                    s32 maxnum = 0x111;
                    if (length - offs < maxnum) maxnum = length - offs;

                    s32 maxback = 0x400;
                    if (offs < maxback) maxback = offs;
                    maxback = (s32)src - maxback;
                    s32 tmpnr;
                    while (maxback <= (s32)ptr) {
                        if (*(u16*)ptr == *(u16*)src && ptr[2] == src[2]) {
                            tmpnr = 3;
                            while (tmpnr < maxnum && ptr[tmpnr] == src[tmpnr]) tmpnr++;
                            if (tmpnr > nr) {
                                if (offs + tmpnr > length) {
                                    nr = length - offs;
                                    back = (s32)(src - ptr);
                                    break;
                                }
                                nr = tmpnr;
                                back = (s32)(src - ptr);
                                if (nr == maxnum) break;
                            }
                        }
                        --ptr;
                    }
                }
                if (nr > 2) {
                    offs += nr;
                    src += nr;
                    if (nr >= 0x12) {
                        dst[pos++] = (u8)(((back -1) >> 8) & 0xF);
                        dst[pos++] = (u8)((back - 1) & 0xFF);
                        dst[pos++] = (u8)((nr - 0x12) & 0xFF);
                        dstOffs +=3;
                    }
                    else {
                        dst[pos++] = (u8)((((back - 1) >> 8) & 0xF) | (((nr - 2) & 0xF) << 4));
                        dst[pos++] = (u8)((back - 1) & 0xFF);
                        dstOffs += 2;
                    }
                    comp = 1;
                }
                else {
                    dst[pos++] = *src++;
                    dstOffs++;
                    offs++;
                }
                header = (u8)((header << 1) | ((comp == 1) ? 0 : 1));
                if (offs >= length) {
                    header = (u8)(header << (7 - i));
                    break;
                }
            }
            dst[headerOffs] = header;
            if (offs >= length) break;
        }
        while ((dstOffs % 4) != 0) dstOffs++;
        File::writeAllBytes(rFilePath, dst, dstOffs);
    }

    u32 encodeSimpleSZS(u8*src, s32 size, s32 pos, u32 *pMatchPos) {
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

    u32 encodeAdvancedSZS(u8* src, s32 size, s32 pos, u32 *pMatchPos)
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
        numBytes = encodeSimpleSZS(src, size, pos, &matchPos);
        *pMatchPos = matchPos;

        if (numBytes >= 3) {
            numBytes1 = encodeSimpleSZS(src, size, pos + 1, &matchPos);
            if (numBytes1 >= numBytes+2) {
                numBytes = 1;
                prevFlag = 1;
            }
        }
        return numBytes;
    }
};