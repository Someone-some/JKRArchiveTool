#include "BinaryWriter.h"
#include "Util.h"

BinaryWriter::BinaryWriter(const std::string &rFileName, EndianSelect endian) {
    mStream = new std::ofstream(rFileName, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    mEndian = endian;
}

BinaryWriter::~BinaryWriter() {
    delete mStream;
}

void BinaryWriter::writeU8(u8 value) {
    mStream->write(reinterpret_cast<const char*>(&value), 1);
}

void BinaryWriter::writeS8(s8 value) {
    mStream->write(reinterpret_cast<const char*>(&value), 1);
}

void BinaryWriter::writeU16(u16 value) {
    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(value);
        
    mStream->write(reinterpret_cast<const char*>(&value), 2);
}

void BinaryWriter::writeS16(s16 value) {
    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(value);
        
    mStream->write(reinterpret_cast<const char*>(&value), 2);
}

void BinaryWriter::writeU32(u32 value) {
    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(value);
        
    mStream->write(reinterpret_cast<const char*>(&value), 4);
}

void BinaryWriter::writeS32(s32 value) {
    if (mEndian == EndianSelect::Big)
        Util::SwapEndian(value);
        
    mStream->write(reinterpret_cast<const char*>(&value), 4);
}

void BinaryWriter::writeString(std::string Str) {
    mStream->write(Str.data(), Str.size());
}

void BinaryWriter::writeBytes(const u8 *bytes, u32 amount) {
    for (s32 i = 0; i < amount; i++) {
        writeU8(bytes[i]);
    }
}

void BinaryWriter::writePadding(u8 value, u32 amount) {
    for (s32 i = 0; i < amount; i++) {
        writeU8(value);
    }
}