#include "Util.h"
#include "BinaryWriter.h"

namespace File {
    void writeAllBytes(const std::string &rFilePath, const u8 *pBytes, u32 bufferSize) {
        BinaryWriter* writer = new BinaryWriter(rFilePath, EndianSelect::Little);
        writer->writeBytes(pBytes, bufferSize);
        writer->~BinaryWriter();
    }

    u8* readAllBytes(const std::string &rFilePath, u32 *byteCount) {
        BinaryReader* reader = new BinaryReader(rFilePath, EndianSelect::Big);
        u8* ret = reader->readAllBytes();
        *byteCount = reader->size();
        reader->~BinaryReader();
        return ret;
    }
};