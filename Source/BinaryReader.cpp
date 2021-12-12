#include "BinaryReader.h"
#include "Util.h"
#include <stdlib.h>
#include <vector>
#include <iostream>

BinaryReader::BinaryReader(const std::string &rFilePath, EndianSelect endian) {
    mStream = new std::ifstream(rFilePath, std::ifstream::in | std::ifstream::binary);
    mEndian = endian;
}

BinaryReader::BinaryReader(const u8 *pBuffer, u32 size, EndianSelect endian) {
    mBuffer = new MemoryBuffer(pBuffer, size);
    mStream = new std::istream(mBuffer);
    mEndian = endian;
}

BinaryReader::~BinaryReader() {
    delete mStream;

    if (mBuffer) 
        delete mBuffer;
}

u8 BinaryReader::readU8() {
    u8 output;
    mStream->read((char*)&output, 1);
    return output;
}

s8 BinaryReader::readS8()  {
    s8 output;
    mStream->read((char*)&output, 1);
    return output;
}

u16 BinaryReader::readU16() {
    u16 output;
    mStream->read((char*)&output, 2);

    if (mEndian == EndianSelect::Big) 
        Util::SwapEndian(output);

    return output; 
}

s16 BinaryReader::readS16() {
    s16 output;
    mStream->read((char*)&output, 2);

    if (mEndian == EndianSelect::Big) 
        Util::SwapEndian(output);

    return output; 
}

u32 BinaryReader::readU32() {
    u32 output;
    mStream->read((char*)&output, 4);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

s32 BinaryReader::readS32() {
    s32 output;
    mStream->read((char*)&output, 4);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

u64 BinaryReader::readU64() {
    u64 output;
    mStream->read((char*)&output, 8);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

s64 BinaryReader::readS64() {
    s64 output;
    mStream->read((char*)&output, 8);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

std::string BinaryReader::readString(const u32 &rLength) {
    std::string output;
    output.reserve(rLength);
    for (s32 i = 0; i < rLength; i++) {
        u8 charBuffer;
        mStream->read((char*)&charBuffer, 1);
        output.push_back(charBuffer);
    }

    if (mEndian == EndianSelect::Big)
        std::reverse(output.end(), output.begin());

    return output;
}

std::string BinaryReader::readNullTerminatedString() {
    std::string output = "";
    while (peekU8() != '\0') {
        u8 buffer;
        mStream->read((char*)&buffer, 1);
        output.push_back(buffer);
    }

    if (mEndian == EndianSelect::Big)
        std::reverse(output.end(), output.begin());
        
    skip(1);
    return output;
}

u8 BinaryReader::peekU8() {
    u8 output = readU8();
    seek(position() -1, std::ios::beg);
    return output;
}

u8* BinaryReader::readBytes(const u32 count) {
    if (mEndian == EndianSelect::Big) {
        seek(position() + (count - 1), std::ios::beg);
        u8* output = new u8[count];

        for (s32 i = 0; i < count; i++) {
            output[i] = readU8();
            seek(position() - 2, std::ios::beg);
        }

        return output;
    }
    else {
        u8* output = new u8[count];

        for (s32 i = 0; i < count; i++) {
            output[i] = readU8();
        }

        return output;
    }

    return nullptr;
}

u8* BinaryReader::readAllBytes() {
    return readBytes(size());
}

void BinaryReader::skip(u64 count) {
    mStream->seekg(count, std::ifstream::cur);
}

void BinaryReader::seek(u32 pos, std::ios::seekdir seekDir) {
    mStream->seekg(pos, seekDir);
}

u64 BinaryReader::position() {
    return mStream->tellg();
}

u64 BinaryReader::size() {
    u64 curPos = position();
    seek(0, std::ios::end);
    u64 endPos = position();

    if(curPos != endPos)
        seek(curPos, std::ios::beg);

    return endPos;
}
