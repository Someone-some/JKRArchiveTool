#include "BinaryReaderAndWriter.h"
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
    while (peek<u8>() != '\0') {
        u8 buffer;
        mStream->read((char*)&buffer, 1);
        output.push_back(buffer);
    }

    if (mEndian == EndianSelect::Big)
        std::reverse(output.end(), output.begin());
        
    skip(1);
    return output;
}

std::string BinaryReader::readNullTerminatedStringAt(const u32 &rPos) {
    u32 curPos = position();
    seek(rPos, std::ios::beg);
    std::string ret = readNullTerminatedString();
    seek(curPos, std::ios::beg);
    return ret;
}

u8* BinaryReader::readBytes(const u32 &count, EndianSelect select) {
    if (mEndian == EndianSelect::Big && select == EndianSelect::Big) {
        u32 curPos = position();
        seek(curPos + (count - 1), std::ios::beg);
        u8* output = new u8[count];

        for (s32 i = 0; i < count; i++) {
            output[i] = read<u8>();
            seek(position() - 2, std::ios::beg);
        }
        
        seek(curPos + count, std::ios::beg);
        return output;
    }
    else {
        u8* output = new u8[count];

        for (s32 i = 0; i < count; i++) {
            output[i] = read<u8>();
        }

        return output;
    }

    return nullptr;
}

u8* BinaryReader::readAllBytes() {
    return readBytes(size());
}

void BinaryReader::skip(u32 count) {
    mStream->seekg(count, std::ifstream::cur);
}

void BinaryReader::seek(u32 pos, std::ios::seekdir seekDir) {
    mStream->seekg(pos, seekDir);
}

u32 BinaryReader::position() {
    return mStream->tellg();
}

u32 BinaryReader::size() {
    u64 curPos = position();
    seek(0, std::ios::end);
    u64 endPos = position();

    if(curPos != endPos)
        seek(curPos, std::ios::beg);

    return endPos;
}

BinaryWriter::BinaryWriter(const std::string &rFileName, EndianSelect endian) {
    mStream = new std::ofstream(rFileName, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    mEndian = endian;
}

BinaryWriter::BinaryWriter(const u8* buffer, u32 size, EndianSelect endian) {
    mBuffer = new MemoryBuffer(buffer, size);
    mStream = new std::ostream(mBuffer);
    mEndian = endian;
}

BinaryWriter::~BinaryWriter() {
    delete mStream;

    if (mBuffer)
        delete[] mBuffer;
}

void BinaryWriter::writeString(std::string Str) {
    mStream->write(Str.data(), Str.size());
}

void BinaryWriter::writeBytes(const u8 *bytes, u32 amount) {
    for (s32 i = 0; i < amount; i++) {
        write<u8>(bytes[i]);
    }
}

void BinaryWriter::writePadding(u8 value, u32 amount) {
    for (s32 i = 0; i < amount; i++) {
        write<u8>(value);
    }
}