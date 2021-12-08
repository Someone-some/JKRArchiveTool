#include "BinaryReader.h"
#include "Util.h"
#include <stdlib.h>

BinaryReader::BinaryReader(const std::string &rFilePath, EndianSelect endian) {
    mStream = new std::ifstream(rFilePath, std::ifstream::in | std::ifstream::binary);
    mEndian = endian;
}

//TODO: reading from memory
BinaryReader::BinaryReader(u8 *pBuffer, u32 size, EndianSelect endian) {
    
}

BinaryReader::~BinaryReader() {
    delete mStream;
}

u8 BinaryReader::readU8() {
    u8 output;
    mStream->read(reinterpret_cast<char*>(&output), 1);
    return output;
}

s8 BinaryReader::readS8()  {
    s8 output;
    mStream->read(reinterpret_cast<char*>(&output), 1);
    return output;
}

u16 BinaryReader::readU16() {
    u16 output;
    mStream->read(reinterpret_cast<char*>(&output), 2);

    if (mEndian == EndianSelect::Big) 
        Util::SwapEndian(output);

    return output; 
}

s16 BinaryReader::readS16() {
    s16 output;
    mStream->read(reinterpret_cast<char*>(&output), 2);

    if (mEndian == EndianSelect::Big) 
        Util::SwapEndian(output);

    return output; 
}

u32 BinaryReader::readU32() {
    u32 output;
    mStream->read(reinterpret_cast<char*>(&output), 4);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

s32 BinaryReader::readS32() {
    s32 output;
    mStream->read(reinterpret_cast<char*>(&output), 4);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

u64 BinaryReader::readU64() {
    u64 output;
    mStream->read(reinterpret_cast<char*>(&output), 8);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

s64 BinaryReader::readS64() {
    s64 output;
    mStream->read(reinterpret_cast<char*>(&output), 8);

    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(output);

    return output;
}

char BinaryReader::readChar() {
    char output;
    mStream->read(&output, 1);
    return output;
}

std::string BinaryReader::readString(const u32 &rLength) {
    std::string output;
    output.reserve(rLength);
    for (int i = 0; i < rLength; i++) {
        char charBuffer;
        mStream->read(&charBuffer, 1);
        output.push_back(charBuffer);
    }

    if (mEndian == EndianSelect::Big)
        std::reverse(output.end(), output.begin());

    return output;
}

std::string BinaryReader::readNullTerminatedString() {
    std::string output;
    char buffer;
    while (readChar() != 0) {
        mStream->read(&buffer, 1);
        output.push_back(buffer);
    }

    if (mEndian == EndianSelect::Big)
        std::reverse(output.end(), output.begin());

    skip(1);
    return output;
}

u8* BinaryReader::readBytes(u32 count) {
    u8* output = new u8[count];

    for (s32 i = 0; i < count; i++) {
        output[i] = readU8();
    }

    return output;
}

void BinaryReader::skip(u64 count) {
    mStream->seekg(count, std::ifstream::cur);
}

void BinaryReader::seek(u64 pos, std::ios::seekdir seekDir) {
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