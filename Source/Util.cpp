#include "Util.h"
#include "BinaryReaderAndWriter.h"

namespace File {
    void writeAllBytes(const std::string &filePath, const u8 *pBytes, u32 bufferSize) {
        BinaryWriter writer(filePath, EndianSelect::Little);
        writer.writeBytes(pBytes, bufferSize);
    }

    u8* readAllBytes(const std::string &filePath, u32 *byteCount) {
        BinaryReader reader(filePath, EndianSelect::Little);
        u8* ret = reader.readAllBytes();
        *byteCount = reader.size();
        return ret;
    }

    bool FileExists(const std::string &filePath) {
        std::ifstream test(filePath);
        if (!test)
            return false;
        else
            return true;
    }
};