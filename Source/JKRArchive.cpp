#include "JKRArchive.h"
#include <direct.h>
#include <dirent.h>
#include "Util.h"

JKRArchive::JKRArchive(const std::string &filePath) {
    BinaryReader* reader = new BinaryReader(filePath, EndianSelect::Big);
    read(*reader);
    reader->~BinaryReader();
}

JKRArchive::JKRArchive(u8*pData, u32 size) {
    BinaryReader* reader = new BinaryReader(pData, size, EndianSelect::Big);
    read(*reader);
    reader->~BinaryReader();
}

void JKRArchive::save(const std::string &filePath) {
    BinaryWriter* writer = new BinaryWriter(filePath, EndianSelect::Big);
    write(*writer);
    writer->~BinaryWriter();
}

void JKRArchive::read(BinaryReader &reader) {
    if (reader.readString(0x4) != "RARC") {
        printf("Fatal error! File is not a valid JKRArchive");
        return;
    }

    mHeader = *reinterpret_cast<const JKRArchiveHeader*>(reader.readBytes(sizeof(JKRArchiveHeader)));
    mDataHeader = *reinterpret_cast<const JKRArchiveDataHeader*>(reader.readBytes(sizeof(JKRArchiveDataHeader)));
    mSyncFileIds = reader.read<u8>() != 0x0;

    reader.seek(mDataHeader.mDirNodeOffset + mHeader.mHeaderSize, std::ios::beg);
    mFolderNodes.reserve(mDataHeader.mDirNodeCount);
    mDirectories.reserve(mDataHeader.mFileNodeCount);

    for (s32 i = 0; i < mDataHeader.mDirNodeCount; i++) {
        // This kinda isn't true but ¯\_(ツ)_/¯
        printf("\rUnpacking Folder %u / %u", i + 1, mDataHeader.mDirNodeCount);
        JKRFolderNode* Node = new JKRFolderNode();
        Node->mNode = *reinterpret_cast<const JKRFolderNode::Node*>(reader.readBytes(sizeof(JKRFolderNode::Node)));
        Node->mName = reader.readNullTerminatedStringAt(mDataHeader.mStringTableOffset + mHeader.mHeaderSize + Node->mNode.mNameOffs);   

        if (!mRoot) {
            Node->mIsRoot = true;
            mRoot = Node;
        }

        mFolderNodes.push_back(Node);
    }
    printf("\n");

    reader.seek(mDataHeader.mFileNodeOffset + mHeader.mHeaderSize, std::ios::beg);

    for (s32 i = 0; i < mDataHeader.mFileNodeCount; i++) {
        JKRDirectory* dir = new JKRDirectory();
        dir->mNode = *reinterpret_cast<const JKRDirectory::Node*>(reader.readBytes(sizeof(JKRDirectory::Node)));
        reader.skip(4); // Skip padding
        u16 nameOffs = dir->mNode.mAttrAndNameOffs & 0x00FFFFFF;
        dir->mAttr = (JKRFileAttr)(dir->mNode.mAttrAndNameOffs >> 24);
        dir->mName = reader.readNullTerminatedStringAt(mDataHeader.mStringTableOffset + mHeader.mHeaderSize + nameOffs);

        if (dir->mName.compare("..") && dir->mName.compare("..") && i < mDataHeader.mFileNodeCount - 1)
            printf("\rUnpacking File %u / %u", i, mDataHeader.mFileNodeCount - 2);

        if (dir->isDirectory() && dir->mNode.mData != 0xFFFFFFFF) {
            dir->mFolderNode = mFolderNodes[dir->mNode.mData];

            if (dir->mFolderNode->mNode.mHash == dir->mNode.mHash)
                dir->mFolderNode->mDirectory = dir;
        }
        else if (dir->isFile()) {
            u32 curPos = reader.position();
            reader.seek(mHeader.mFileDataOffset + mHeader.mHeaderSize + dir->mNode.mData, std::ios::beg);
            u8* pData = reader.readBytes(dir->mNode.mDataSize, EndianSelect::Little);
            reader.seek(curPos, std::ios::beg);

            if (dir->getCompressionType() == JKRCompressionType_SZS)
                pData = JKRCompression::decodeSZS(pData, dir->mNode.mDataSize);
            else if (dir->getCompressionType() == JKRCompressionType_SZP)
                pData = JKRCompression::decodeSZP(pData, dir->mNode.mDataSize);
                
            dir->mData = pData;
        }

        mDirectories.push_back(dir);
    }
    printf("\n");

    for (s32 i = 0; i < mFolderNodes.size(); i++) {
        JKRFolderNode* node = mFolderNodes[i];

        for (s32 y = node->mNode.mFirstFileOffs; y < (node->mNode.mFirstFileOffs + node->mNode.mFileCount); y++) {  
            JKRDirectory* childDir = mDirectories[y];
            childDir->mParentNode = node;
            node->mChildDirs.push_back(childDir);
        }
    }
}

void JKRArchive::write(BinaryWriter &writer) {
    writer.writeString("RARC");
}

void JKRArchive::sortNodesAndDirs() {
    for (s32 i = 0; i < mDirectories.size(); i++) {
        if (mDirectories[i]->isDirectory())  {
            if (mDirectories[i]->mFolderNode)
                mDirectories[i]->mNode.mData = getNodeIndex(mDirectories[i]->mFolderNode);
            else    
                mDirectories[i]->mNode.mData = 0xFFFFFFFF;
        }
        else {
            if (mSyncFileIds)
                mDirectories[i]->mNode.mNodeIdx = getDirIndex(mDirectories[i]);

            if (mDirectories[i]->getPreloadType() == JKRPreloadType_MRAM)
                mMRAMFiles.push_back(mDirectories[i]);
            else if (mDirectories[i]->getPreloadType() == JKRPreloadType_ARAM)
                mARAMFiles.push_back(mDirectories[i]);
            else if (mDirectories[i]->getPreloadType() == JKRPreloadType_DVD)
                mDVDFiles.push_back(mDirectories[i]);
        }
    }
}

void JKRArchive::unpack(const std::string &filePath) {
    std::string fullpath;
    fullpath = filePath + "/" + mRoot->mName;
    mkdir(fullpath.c_str());
    mRoot->unpack(fullpath);
}

void JKRArchive::importFromFolder(const std::string &filePath) {
    mRoot = new JKRFolderNode();
    mRoot->mIsRoot = true;
    mRoot->importFromFolder(filePath, *this);
}

void JKRFolderNode::unpack(const std::string &filePath) {
    std::string fullpath;
    for (s32 i = 0; i < mChildDirs.size(); i++) {
        
        if (mChildDirs[i]->mName == "." || mChildDirs[i]->mName == "..")
            continue;

        fullpath = filePath + "/" + mChildDirs[i]->mName;

        if (mChildDirs[i]->isDirectory()) {      
            mkdir(fullpath.c_str());
            mChildDirs[i]->mFolderNode->unpack(fullpath);
        }
        else if (mChildDirs[i]->isFile()) {
            File::writeAllBytes(fullpath, mChildDirs[i]->mData, mChildDirs[i]->mNode.mDataSize);
        }
    }
}

void JKRFolderNode::importFromFolder(const std::string &filePath, const JKRArchive &archive) {
    DIR* dir = opendir(filePath.c_str());
    dirent* ent;

    while (ent = readdir(dir)) {
        if(ent->d_type == DT_REG) {
            JKRDirectory* file = new JKRDirectory();
            file->mName = ent->d_name;
            file->mData = File::readAllBytes(filePath + "/" + file->mName, &file->mNode.mDataSize);
            mChildDirs.push_back(file);
        }
        else if (ent->d_type = DT_DIR &&) {
            JKRDirectory* file = new JKRDirectory();   
            file->mAttr = JKRFileAttr_FOLDER; 
        }
    }
}

JKRDirectory::JKRDirectory() {
    mAttr = JKRFileAttr_FILE;
    mFolderNode = nullptr;
    mParentNode = nullptr;
    mName = "";
    mData = nullptr;
}

JKRCompressionType JKRDirectory::getCompressionType() {
    if (mAttr & JKRFileAttr_FILE && mAttr & JKRFileAttr_COMPRESSED) {
        if (mAttr & JKRFileAttr_USE_YAZ0) 
            return JKRCompressionType_SZS;
        else 
            return JKRCompressionType_SZP;
    }

    return JKRCompressionType_NONE;
}

JKRPreloadType JKRDirectory::getPreloadType() {
    if (isFile()) {
        if (mAttr & JKRFileAttr_LOAD_TO_MRAM)
            return JKRPreloadType_MRAM;
        else if (mAttr & JKRFileAttr_LOAD_TO_ARAM)
            return JKRPreloadType_ARAM;
        else if (mAttr & JKRFileAttr_LOAD_FROM_DVD) {
            return JKRPreloadType_DVD;
        }
    }

    return JKRPreloadType_NONE;
}