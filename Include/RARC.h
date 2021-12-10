#pragma once

#include "types.h"
#include "BinaryReader.h"
#include <string>
#include <vector>

class RARC {
public:
    RARC(const std::string &);
    RARC(u8*, u32);

    ~RARC();
    
    void exportContents(const std::string &);
    u8* getFile(const std::string &, u32 *);

    struct FileNode {
        u32 mEntryOffset;
        u32 mEntryId;
        u32 mNameOffset;
        u32 mDataOffset;
        u32 mDataSize;
        u32 mParentDirId;
        std::string mName;  
        std::string mFullName;
    };

    struct DirNode {
        u32 mEntryOffset;
        u32 mEntryId;
        u32 mNameOffset;
        u32 mDataOffset;
        u32 mParentId;
        std::string mName; 
        std::string mFullName;
    };

    std::vector<DirNode*> mDirNodes;
    std::vector<FileNode*> mFileNodes;
private:
    void read(BinaryReader&);

    u32 mFileDataOffset = 0;
    u32 mDirNodeCount;
    u32 mDirNodeOffset;
    u32 mFileEntriesOffset;
    u32 mStringTableOffset;
    BinaryReader* mReader;
};