#include "Util.h"
#include "BinaryReaderAndWriter.h"

namespace File {
    void writeAllBytes(const std::string &filePath, const u8 *pBytes, u32 bufferSize) {
        BinaryWriter* writer = new BinaryWriter(filePath, EndianSelect::Little);
        writer->writeBytes(pBytes, bufferSize);
        writer->~BinaryWriter();
    }

    u8* readAllBytes(const std::string &filePath, u32 *byteCount) {
        BinaryReader* reader = new BinaryReader(filePath, EndianSelect::Little);
        u8* ret = reader->readAllBytes();
        *byteCount = reader->size();
        reader->~BinaryReader();
        return ret;
    }
};