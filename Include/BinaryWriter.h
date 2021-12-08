#pragma once

#include <string>
#include <fstream>
#include "BinaryReader.h"

class BinaryWriter {
public:
    BinaryWriter(const std::string &, EndianSelect);
    BinaryWriter(u8*, EndianSelect);

    ~BinaryWriter();

    void writeU8(u8);
    void writeS8(s8);
    void writeU16(u16);
    void writeS16(s16);
    void writeU32(u32);
    void writeS32(s32);
    void writeBytes(const u8*);

    EndianSelect mEndian;
private:
    std::ofstream* mStream = nullptr;
};