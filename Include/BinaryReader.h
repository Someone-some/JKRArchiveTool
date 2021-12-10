#pragma once

#include <fstream>
#include <string>
#include <string.h>
#include <streambuf>
#include <algorithm>
#include <numeric>
#include <limits>
#include "types.h"

enum EndianSelect {
    Big,
    Little
};

class MemoryBuffer : public std::streambuf {
public:
    MemoryBuffer(u8*pBuffer, u32 size) : std::streambuf() {
        mBuffer = pBuffer;
        mSize = size;
        setg((char*)pBuffer, (char*)pBuffer, (char*)(pBuffer + size));
    }

    u32 mSize;
    u8* mBuffer;

protected: 
    virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override {
        if (which != std::ios_base::in) {
            throw std::invalid_argument("basic_memstreambuf::seekoff[which]");
        }

        if (dir == std::ios_base::beg) {
            if (off >= 0 && off < egptr() - eback()) {
                setg(eback(), eback() + off, egptr());
            }
            else {
                throw std::out_of_range("basic_memstreambuf::seekoff[beg]");
            }
        }
        else if (dir == std::ios_base::cur) {
            if ((off >= 0 && off <= egptr() - gptr()) || (off < 0 && std::abs(off) < gptr() - eback())) {
                gbump((int)off);
            }
            else {
                throw std::out_of_range("basic_memstreambuf::seekoff[cur]");
            }
        }
        else if (dir == std::ios_base::end) {
            if (off <= 0 && std::abs(off) < egptr() - eback()) {
                setg(eback(), egptr() + (int)off, egptr());
            }
            else {
                throw std::out_of_range("basic_memstreambuf::seekoff[end]");
            }
        }
        else {
            throw std::invalid_argument("basic_memstreambuf::seekoff[dir]");
        }

        return gptr() - eback();
    }

    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override {
        if (which != std::ios_base::in) {
            throw std::invalid_argument("basic_memstreambuf::seekpos[which]");
        }

        if (pos < egptr() - eback()) {
            setg(eback(), eback() + pos, egptr());
        }
        else {
            throw std::out_of_range("memstreambuf::seekpos");
        }

        return pos;
    }
};

class BinaryReader {
public:
    BinaryReader(const std::string &, EndianSelect);
    BinaryReader(u8*, u32, EndianSelect);

    ~BinaryReader();

    u8 readU8();
    s8 readS8();
    u16 readU16();
    s16 readS16();
    u32 readU32();
    s32 readS32();
    u64 readU64();
    s64 readS64();
    std::string readString(const u32 &);
    std::string readNullTerminatedString();

    u8 peekU8();
    u8* readBytes(u32 count);
    u8* readAllBytes();
    void close();
    void skip(u64);
    void seek(u32, std::ios::seekdir);
    u64 position();
    u64 size();

    EndianSelect mEndian;
private:
    MemoryBuffer* mBuffer = nullptr;
    std::istream* mStream = nullptr;
};