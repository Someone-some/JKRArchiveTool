#pragma once

#include <fstream>
#include <string>
#include <string.h>
#include <streambuf>
#include <algorithm>
#include <numeric>
#include <limits>
#include "Util.h"
#include "types.h"

enum EndianSelect {
    Big,
    Little
};

class MemoryBuffer : public std::streambuf {
public:
    MemoryBuffer(const u8*pBuffer, const u32 size) : std::streambuf() {
        mBuffer = pBuffer;
        mSize = size;
        setg((char*)pBuffer, (char*)pBuffer, (char*)(pBuffer + size));
    }

    u32 mSize;
    const u8* mBuffer;

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

namespace {
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

class BinaryReader {
public:
    BinaryReader(const std::string &, EndianSelect);
    BinaryReader(const u8 *, u32, EndianSelect);

    ~BinaryReader();

    template<typename T> 
    T read() {
        T output; 
        mStream->read((char*)&output, sizeof(T));

        if (mEndian == EndianSelect::Big && sizeof(T) > 1) 
            SwapEndian(output);

        return output;
    }

    std::string readString(const u32 &);
    std::string readNullTerminatedString();
    std::string readNullTerminatedStringAt(const u32 &);

    template<typename T> 
    T peek() {
        T output = read<T>();
        seek(position() - sizeof(T), std::ios::beg);
        return output;
    }

    u8* readBytes(const u32 &, EndianSelect = EndianSelect::Big);
    u8* readAllBytes();
    void close();
    void skip(u32);
    void seek(u32, std::ios::seekdir);
    u32 position();
    u32 size();

    EndianSelect mEndian;
private:
    MemoryBuffer* mBuffer = nullptr;
    std::istream* mStream = nullptr;
};

class BinaryWriter {
public:
    BinaryWriter(const std::string &, EndianSelect);
    BinaryWriter(const u8*, u32, EndianSelect);

    ~BinaryWriter();

    template<typename T>
    void write(T val) {
        if (mEndian == EndianSelect::Big && sizeof(T) > 1)
            SwapEndian(val);
        
        mStream->write(reinterpret_cast<const char*>(&val), sizeof(T));
    }

    void writeString(std::string);

    void writeBytes(const u8*, u32);
    void writePadding(u8, u32);

    EndianSelect mEndian;
private:
    MemoryBuffer* mBuffer = nullptr;
    std::ostream* mStream = nullptr;
};