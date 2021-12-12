#include "JKRArchive.h"
#include <iostream>

JKRArchive::JKRArchive(const std::string &rFileName) {
    BinaryReader* reader = new BinaryReader(rFileName, EndianSelect::Big);
    read(*reader);
    reader->~BinaryReader();
}

JKRArchive::JKRArchive(u8*pData, u32 size) {
    BinaryReader* reader = new BinaryReader(pData, size, EndianSelect::Big);
    read(*reader);
    reader->~BinaryReader();
}

void JKRArchive::read(BinaryReader &rReader) {
    if (rReader.readString(0x4) != "RARC") {
        printf("Fatal error! File is not a valid JKRArchive");
        return;
    }

    mHeader = *reinterpret_cast<const JKRArchiveHeader*>(rReader.readBytes(0x1C));
    mDataHeader = *reinterpret_cast<const JKRArchiveDataHeader*>(rReader.readBytes(0x1A)); 
}