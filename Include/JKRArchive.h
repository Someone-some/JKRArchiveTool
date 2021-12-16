#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "JKRCompression.h"
#include <vector>

// Heavily based off https://github.com/SunakazeKun/pygapa/blob/main/jsystem/jkrarchive.py

enum JKRFileAttr {
    JKRFileAttr_FILE = 0x1,
    JKRFileAttr_FOLDER = 0x2,
    JKRFileAttr_COMPRESSED = 0x4,
    JKRFileAttr_LOAD_TO_MRAM = 0x10,
    JKRFileAttr_LOAD_TO_ARAM = 0x20,
    JKRFileAttr_LOAD_FROM_DVD = 0x40,
    JKRFileAttr_USE_YAZ0 = 0x80,
};

enum JKRPreloadType {
    JKRPreloadType_NONE = -1,
    JKRPreloadType_MRAM = 0,
    JKRPreloadType_ARAM = 1,
    JKRPreloadType_DVD = 2,
};

struct JKRArchiveHeader {
    u32 mDVDFileSize;
    u32 mARAMSize;
    u32 mMRAMSize;
    u32 mFileDataSize;
    u32 mFileDataOffset;
    u32 mHeaderSize;
    u32 mFileSize;
};

struct JKRArchiveDataHeader {
    u32 mStringTableOffset;
    u32 mStringTableSize;
    u32 mFileNodeOffset;
    u32 mFileNodeCount;
    u32 mDirNodeOffset;
    u32 mDirNodeCount;
};

class JKRDirectory;

class JKRFolderNode {
public:
    JKRFolderNode() {}

    struct Node {
        u32 mFirstFileOffs;
        u16 mFileCount;
        u16 mHash;
        u32 mNameOffs;
        u8 mShortName[4];
    };

    Node mNode;
    bool mIsRoot = false; 
    std::string mName;
    JKRDirectory* mDirectory;
    std::vector<JKRDirectory*> mChildDirs;
};

class JKRDirectory {
public:
    JKRDirectory() {}

    struct Node {
        u32 mDataSize;
        u32 mData; 
        u32 mAttrAndNameOffs;
        u16 mHash;
        u16 mNodeIdx;
    };

    JKRCompressionType getCompressionType();

    JKRFileAttr mAttr;
    Node mNode;
    JKRFolderNode* mFolderNode;
    JKRFolderNode* mParentNode;
    std::string mName;
    u8* mData;
};

class JKRArchive {
public:
    JKRArchive(const std::string &);
    JKRArchive(u8*, u32);

    void unpack(const std::string &);

private:
    void read(BinaryReader &);

    JKRArchiveHeader mHeader;
    JKRArchiveDataHeader mDataHeader;
    std::vector<JKRFolderNode*> mFolderNodes;
    std::vector<JKRDirectory*> mDirectories;

    JKRFolderNode* mRoot = nullptr;
    bool mSyncFileIds;
};
