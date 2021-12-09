#include "RARC.h"
#include "Util.h"
#include <direct.h>

RARC::RARC(const std::string &rFileName) {
    mReader = new BinaryReader(rFileName, EndianSelect::Big);
    read(*mReader);
}

void RARC::read(BinaryReader &rReader) {
    if (rReader.readString(0x4) != "RARC") {
        printf("Invalid identifier! Expected: RARC");
        return;
    }

    rReader.seek(0xC, std::ios::beg);
    mFileDataOffset = rReader.readU32() + 0x20;
    rReader.seek(0x20, std::ios::beg);
    mDirNodeCount = rReader.readU32();
    mDirNodeOffset = rReader.readU32() + 0x20;
    rReader.skip(0x4);
    mFileEntriesOffset = rReader.readU32() + 0x20;
    rReader.skip(0x4);
    mStringTableOffset = rReader.readU32() + 0x20;

    DirNode* rootDir = new DirNode();
    rootDir->mEntryId = 0;
    rootDir->mParentId = 0xFFFFFFFF;

    rReader.seek(mDirNodeOffset + 0x6, std::ios::beg);
    u32 rootDirOffset = rReader.readU16();
    rReader.seek(mStringTableOffset + rootDirOffset, std::ios::beg);
    rootDir->mName = rReader.readNullTerminatedString();
    rootDir->mFullName = "/" + rootDir->mName;
    mDirNodes.push_back(rootDir);

    for (s32 i = 0; i < mDirNodeCount; i++) {
        DirNode* dir = mDirNodes[i];
        rReader.seek(mDirNodeOffset + (i * 0x10) + 10, std::ios::beg);
        u16 entryCount = rReader.readU16();
        u32 firstEntry = rReader.readU32();

        for (u32 y = 0; y < entryCount; y++) {
            u32 entryOffset = mFileEntriesOffset + ((y + firstEntry) * 0x14);   
            rReader.seek(entryOffset, std::ios::beg);

            u16 id = rReader.readU16();
            rReader.skip(0x4);
            u16 nameOffset = rReader.readU16();
            s32 dataOffset = rReader.readS32();
            u32 dataSize = rReader.readU32();

            rReader.seek(mStringTableOffset + nameOffset, std::ios::beg);
            std::string name = rReader.readNullTerminatedString();

            if (name == "." || name == "..")
                continue;

            std::string fullName = dir->mFullName + "/" + name;

            if (id == 0xFFFF) {
                DirNode* dir = new DirNode();
                dir->mEntryOffset = entryOffset;
                dir->mEntryId = dataOffset;
                dir->mParentId = i;
                dir->mNameOffset = nameOffset;
                dir->mName = name;
                dir->mFullName = fullName;

                mDirNodes.push_back(dir);
            }
            else {
                FileNode* file = new FileNode();
                file->mEntryOffset = entryOffset;
                file->mEntryId = id;
                file->mParentDirId = i;
                file->mNameOffset = nameOffset;
                file->mDataOffset = mFileDataOffset + dataOffset;
                file->mDataSize = dataSize;
                file->mName = name;
                file->mFullName = fullName;     

                mFileNodes.push_back(file);
            }
        }
    }
}

RARC::~RARC() {
    delete mReader;
}

void RARC::ExportContents(const std::string &rFilePath) {
    std::string FullPath;
    for (s32 i = 0; i < mDirNodes.size(); i++) {
        FullPath = rFilePath + mDirNodes[i]->mFullName;
        _mkdir(FullPath.c_str());
    }

    for (s32 y = 0; y < mFileNodes.size(); y++) {
        FullPath = rFilePath + mFileNodes[y]->mFullName;
        u32 size;
        u8* file = getFile(mFileNodes[y]->mFullName, &size);
        File::writeAllBytes(FullPath, file, size);
    }
}

u8* RARC::getFile(const std::string &rFilePath, u32 *size) {
    for (s32 i = 0; i < mFileNodes.size(); i++) {
        if (mFileNodes[i]->mFullName == rFilePath) {
            mReader->seek(mFileNodes[i]->mDataOffset, std::ios::beg);
            *size = mFileNodes[i]->mDataSize;
            return mReader->readBytes(mFileNodes[i]->mDataSize);
        }
    }

    printf("File not found\n");
}