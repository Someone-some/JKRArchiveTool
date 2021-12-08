#include "Util.h"
#include "BinaryWriter.h"

namespace File {
    void writeAllBytes(const std::string &rFilePath, const u8 *pBytes) {
        BinaryWriter* writer = new BinaryWriter(rFilePath, EndianSelect::Little);
        printf("Writing\n");
        writer->writeBytes(pBytes);
        writer->~BinaryWriter();
    }
};