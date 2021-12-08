#pragma once

#include "types.h"
#include <string>

class RARC {
public:
    struct Header {
        u8 mMagic[4];
        u32 mSize;
        u32 mDataOffset;    
        u32 mFileDataOffset; 
        u32 mFileDataLength;
        u32 mSizeOfMRAMFiles;
        u32 mSizeOfARAMFiles;
        u32 mSizeOfDVDFiles;
    };

    struct DataHeader {
        u32 mDirCount;
        u32 mOffsetToDirs;
        u32 mFileCount;
        u32 mOffsetToFiles;
        u32 mStringTableSize;
        u32 mStringTableOffset;
        u16 mNextFileIndex;
        bool mKeepFileIdSync;
        u16 mPadding[5];
    };

    struct DirNode {
        std::string mDirName; // only first 4 chars
        u32 mNameStringTableOffset;
        u16 mHash;
        u16 mFileCount;
        u32 mFirstFileOffset;
    };

    struct FileNode {
        
    };
};