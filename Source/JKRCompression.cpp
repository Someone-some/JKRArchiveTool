#include "JKRCompression.h"
#include "Util.h"
#include <iostream>

namespace JKRCompression {
    JKRCompressionType checkCompression(const std::string &filePath) {
        BinaryReader reader(filePath, EndianSelect::Big);
        std::string magic = reader.readString(0x4);

        if (magic == "Yaz0") {
            printf("SZS compression found!\n");
            return JKRCompressionType_SZS;
        }      
        else if (magic == "Yay0") {
            printf("SZP compression found!\n");
            return JKRCompressionType_SZP;
        }         
        else {
            reader.seek(0, std::ios::beg);

            if (reader.readString(0x3) == "ASR") {
                printf("ASR compression found!\n");
                return JKRCompressionType_ASR;
            }     
        }
        printf("No compression found!\n");
        return JKRCompressionType_NONE;
    }

    u8* decode(const std::string &filePath, u32 *bufferSize) {
        JKRCompressionType compType = checkCompression(filePath);
        u32 size;
        *bufferSize = size;
        u8* pData = File::readAllBytes(filePath, &size);

        switch (compType) {
            case JKRCompressionType_NONE:
                return nullptr;
            case JKRCompressionType_SZP:
                printf("Decompressing!\n");
                return decodeSZP(pData, size);
            case JKRCompressionType::JKRCompressionType_SZS:
                printf("Decompressing!\n");
                return decodeSZS(pData, size);
            case JKRCompressionType_ASR:
                printf("Compression type: JKRCompressionType_ASR not supported!\n");
                exit(1);
        }

        return nullptr;
    }

    void encode(const std::string &filePath, JKRCompressionType CompType, bool fast) {
        u32 srcSize;
        u32 dstSize;
        u8* src = File::readAllBytes(filePath, &srcSize);
        const u8* dst;

        switch (CompType) {
            case JKRCompressionType_SZS:
                    dst = encodeSZSFast(src, srcSize, &dstSize);
                    break;
            case JKRCompressionType_SZP: 
                if (fast)
                    printf("Fast compression doesn't exist for JKRCompressionType_SZP! Using normal compression\n");
                encodeSZP(filePath);
                break;
            case JKRCompressionType_ASR:
                printf("Compression type: JKRCompressionType_ASR not supported!\n");
                exit(1);
                break;
        }

        File::writeAllBytes(filePath, dst, dstSize);
    }
    
    u8* decodeSZS(const u8*pData, u32 bufferSize) {
        BinaryReader reader(pData, bufferSize, EndianSelect::Big);
        reader.skip(0x4);

        u32 decompSize = reader.read<u32>();
        reader.skip(0x8);
        u8* dst = new u8[decompSize];
        u32 dstPos = 0;

        u32 validBitCount = 0;
        u8 block = 0;

        while (dstPos < bufferSize) {
            if (validBitCount == 0) {
                block = reader.read<u8>();
                validBitCount = 8;
            }

            if ((block & 0x80) != 0) {
                dst[dstPos] = reader.read<u8>();
                dstPos++;
            }
            else {
                u8 byte1 = reader.read<u8>();
                u8 byte2 = reader.read<u8>();

                u32 copySrc = dstPos - ((((byte1 & 0xF) << 8) | byte2) + 1);
                u32 numBytes = byte1 >> 4;

                if (numBytes == 0) 
                    numBytes = reader.read<u8>() + 0x12;
                else 
                    numBytes += 2;

                for (s32 i = 0; i < numBytes; i++) {
                    dst[dstPos] = dst[copySrc];
                    copySrc++;
                    dstPos++;
                }
            }

            block <<= 1;
            validBitCount--;
        }

        return dst;
    }

    u8* decodeSZP(const u8*pData, u32 bufferSize) {
        BinaryReader* reader = new BinaryReader(pData, bufferSize, EndianSelect::Big);
        if (reader->readString(0x4) != "Yay0") {
            printf("Invalid identifier! Expected Yay0\n");
            return nullptr;
        }

        u32 decompSize = reader->read<u32>();
        u32 linkTableOffs = reader->read<u32>();
        u32 byteChunkAndCountModiferOffset = reader->read<u32>();

        u8* dst = new u8[decompSize];
        s32 maskedBitCount = 0;
        s32 curOffsInDst = 0;
        s32 curMask = 0;

        while (curOffsInDst < decompSize) {
            if (maskedBitCount == 0) {
                curMask = reader->read<s32>();
                maskedBitCount = 32;
            }

            if (((u32)curMask & (u32)0x80000000) == 0x80000000) {
                u32 curPos = reader->position();
                reader->seek(byteChunkAndCountModiferOffset++, std::ios::beg);
                dst[curOffsInDst++] = reader->read<u8>();
                reader->seek(curPos, std::ios::beg);
            }
            else {
                u64 curPos = reader->position();
                reader->seek(linkTableOffs++, std::ios::beg);
                u16 link = reader->read<u16>();
                linkTableOffs += 2;
                reader->seek(curPos, std::ios::beg);

                s32 offset = curOffsInDst - (link & 0xFFF);
                s32 count = link >> 0xC;

                if (count == 0) {
                    u64 curPos = reader->position();
                    reader->seek(byteChunkAndCountModiferOffset++, std::ios::beg);
                    u8 countModifer = reader->read<u8>();
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

    const u8* encodeSZS(u8* src, u32 srcSize, u32 *outSize) {
        BinaryWriter writer(src, srcSize, EndianSelect::Little);
        writer.writeString("Yaz0");
        return writer.getBuffer();
        writer.write<u32>(srcSize);
        writer.writePadding(0x0, 8);
        u8 dst[24];
        s32 srcPos = 0;
        s32 dstPos = 0;
        s32 dstSize = 0;
        s32 percent = 0;

        u32 validBitCount = 0;
        u8 currCodeByte = 0;
        while(srcPos < srcSize) {
            u32 numBytes;
            u32 matchPos;
            u32 srcPosBak;

            numBytes = encodeAdvancedSZS(src, srcSize, srcPos, &matchPos);
            if (numBytes < 3) {
                dst[dstPos] = src[srcPos];
                dstPos++;
                srcPos++;
                currCodeByte |= (0x80 >> validBitCount);
            }
            else {
                u32 dist = srcPos - matchPos - 1; 
                u8 byte1, byte2, byte3;

                if (numBytes >= 0x12) {
                    byte1 = 0 | (dist >> 8);
                    byte2 = dist & 0xff;
                    dst[dstPos++] = byte1;
                    dst[dstPos++] = byte2;

                    if (numBytes > 0xff + 0x12)
                        numBytes = 0xff + 0x12;
                        
                    byte3 = numBytes - 0x12;
                    dst[dstPos++] = byte3;
                } 
                else {
                    byte1 = ((numBytes - 2) << 4) | (dist >> 8);
                    byte2 = dist & 0xff;
                    dst[dstPos++] = byte1;
                    dst[dstPos++] = byte2; 
                }
                srcPos += numBytes;
            }
            validBitCount++;

            if (validBitCount == 8) {
                writer.write<u8>(currCodeByte);

                writer.writeBytes(dst, dstPos);
                dstSize += dstPos + 1;

                currCodeByte = 0;
                validBitCount = 0;
                dstPos = 0;
            }

            if ((srcPos + 1) * 100 / srcSize != percent) {
                percent = (srcPos + 1) * 100 / srcSize;
                printf("\rProgress: %u%%", percent);
            }
        }
        if (validBitCount > 0) {
            writer.write<u8>(currCodeByte);
            writer.writeBytes(dst, dstPos);
            dstSize += dstPos + 1;

            currCodeByte = 0;
            validBitCount = 0;
            dstPos = 0;
        }
        printf("\n");
        *outSize = dstSize;
        std::cout << src[0] << '\n';
        return writer.getBuffer();
    }

    // This is faster, but the files it produces are larger
    const u8* encodeSZSFast(u8*src, u32 srcSize, u32 *pDstSize) {
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
                    maxback = *(s32*)src - maxback;
                    s32 tmpnr;
                    while (maxback <= *(s32*)ptr) {
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
        *pDstSize = dstOffs;
        return dst;
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

    void encodeSZP(const std::string &filePath) {
        printf("Compression type: JKRCompressionType_SZP not implemented!\n");
        exit(1);
        u32 size;
        u8* src = File::readAllBytes(filePath, &size);
        BinaryWriter* writer = new BinaryWriter(filePath, EndianSelect::Big);
        writer->writeString("Yay0");
        writer->write<u32>(size);
        u32 srcPos = 0;
        u32 dstPos = 0;

        while (srcPos < size) {
            
        }
    }
};