#pragma once

#include <array>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <streambuf>
#include <numeric>
#include <limits>
#include "types.h"

enum EndianSelect {
    Big,
    Little
};

class BinaryReader {
public:
    BinaryReader(const std::string &, EndianSelect);
    BinaryReader(s8*, u32, EndianSelect);

    ~BinaryReader();

    u8 readU8();
    s8 readS8();
    u16 readU16();
    s16 readS16();
    u32 readU32();
    s32 readS32();
    u64 readU64();
    s64 readS64();
    char readChar();
    std::string readString(const u32 &);
    std::string readNullTerminatedString();

    void close();
    void skip(u64);
    void seek(u64, std::ios::seekdir);
    u64 position();
    u64 size();

    EndianSelect mEndian;
private:
    u64 mPosition = 0;
    std::istream* mStream = nullptr;
};

namespace Util {
    template <typename T>
    void SwapEndian(T &val) {
        union U {
            T val;
            std::array<u8, sizeof(T)> raw;
        } src, dst;

        src.val = val;
        std::reverse_copy(src.raw.begin(), src.raw.end(), dst.raw.begin());
        val = dst.val;
    }
};