#include "BinaryReader.h"
#include "BinaryWriter.h"

struct JKRArchiveHeader {
    u32 mDVDFileSize;
    u32 mARAMSize;
    u32 mMRAMSize;
    u32 mFileDataSize;
    u32 mFileDataOffset;
    u32 mHeaderLength;
    u32 mFileSize;
};

struct JKRArchiveDataHeader {
    bool mKeepFileIdsSynced;
    u16 mNextFreeFileIdx;
    u32 mStringTableOffset;
    u32 mStringTableSize;
    u32 mFileNodeOffset;
    u32 mFileNodeCount;
    u32 mDirNodeOffset;
    u32 mDirNodeCount;
};

class JKRArchive {
public:
    JKRArchive(const std::string &);
    JKRArchive(u8*, u32);

private:
    void read(BinaryReader &);
    JKRArchiveHeader mHeader;
    JKRArchiveDataHeader mDataHeader;
};