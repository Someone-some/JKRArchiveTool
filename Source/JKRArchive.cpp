#include "JKRArchive.h"
#include <direct.h>
#include <iostream>
#include "Util.h"

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

    mHeader = *reinterpret_cast<const JKRArchiveHeader*>(rReader.readBytes(sizeof(JKRArchiveHeader)));
    mDataHeader = *reinterpret_cast<const JKRArchiveDataHeader*>(rReader.readBytes(sizeof(JKRArchiveDataHeader)));
    mSyncFileIds = rReader.readU8() != 0x0;

    rReader.seek(mDataHeader.mDirNodeOffset + mHeader.mHeaderSize, std::ios::beg);
    mFolderNodes.reserve(mDataHeader.mDirNodeCount);
    mDirectories.reserve(mDataHeader.mFileNodeCount);

    for (s32 i = 0; i < mDataHeader.mDirNodeCount; i++) {
        JKRFolderNode* Node = new JKRFolderNode();
        Node->mNode = *reinterpret_cast<const JKRFolderNode::Node*>(rReader.readBytes(sizeof(JKRFolderNode::Node)));
        Node->mName = rReader.readNullTerminatedStringAt(mDataHeader.mStringTableOffset + mHeader.mHeaderSize + Node->mNode.mNameOffs);   

        if (!mRoot) {
            Node->mIsRoot = true;
            mRoot = Node;
        }

        mFolderNodes.push_back(Node);
    }

    rReader.seek(mDataHeader.mFileNodeOffset + mHeader.mHeaderSize, std::ios::beg);

    for (s32 i = 0; i < mDataHeader.mFileNodeCount; i++) {
        JKRDirectory* dir = new JKRDirectory();
        dir->mNode = *reinterpret_cast<const JKRDirectory::Node*>(rReader.readBytes(sizeof(JKRDirectory::Node)));
        rReader.skip(4); // Skip padding
        u16 nameOffs = dir->mNode.mAttrAndNameOffs & 0x00FFFFFF;
        dir->mAttr = (JKRFileAttr)(dir->mNode.mAttrAndNameOffs >> 24);
        dir->mName = rReader.readNullTerminatedStringAt(mDataHeader.mStringTableOffset + mHeader.mHeaderSize + nameOffs);

        if (dir->isDirectory() && dir->mNode.mData != 0xFFFFFFFF) {
            dir->mFolderNode = mFolderNodes[dir->mNode.mData];

            if (dir->mFolderNode->mNode.mHash == dir->mNode.mHash)
                dir->mFolderNode->mDirectory = dir;
        }
        else if (dir->isFile()) {
            u32 curPos = rReader.position();
            rReader.seek(mHeader.mFileDataOffset + mHeader.mHeaderSize + dir->mNode.mData, std::ios::beg);
            u8* pData = rReader.readBytes(dir->mNode.mDataSize);
            rReader.seek(curPos, std::ios::beg);

            if (dir->getCompressionType() == JKRCompressionType_SZS)
                pData = JKRCompression::decodeSZS(pData, dir->mNode.mDataSize);
            else if (dir->getCompressionType() == JKRCompressionType_SZP)
                pData = JKRCompression::decodeSZP(pData, dir->mNode.mDataSize);
                
            dir->mData = pData;
        }

        mDirectories.push_back(dir);
    }

    for (s32 i = 0; i < mFolderNodes.size(); i++) {
        JKRFolderNode* node = mFolderNodes[i];

        for (s32 y = node->mNode.mFirstFileOffs; y < (node->mNode.mFirstFileOffs + node->mNode.mFileCount); y++) {  
            JKRDirectory* childDir = mDirectories[y];
            childDir->mParentNode = node;
            node->mChildDirs.push_back(childDir);
        }
    }
}

void JKRArchive::unpack(const std::string &rFilePath) {
    std::string fullpath;
    fullpath = rFilePath + "/" + mRoot->mName;
    mkdir(fullpath.c_str());
    mRoot->unpack(fullpath);
}

void JKRFolderNode::unpack(const std::string &rFilePath) {
    std::string fullpath;
    for (s32 i = 0; i < mChildDirs.size(); i++) {
        
        if (mChildDirs[i]->mName == "." || mChildDirs[i]->mName == "..")
            continue;

        fullpath = rFilePath + "/" + mChildDirs[i]->mName;

        if (mChildDirs[i]->isDirectory()) {      
            mkdir(fullpath.c_str());
            mChildDirs[i]->mFolderNode->unpack(fullpath);
        }
        else if (mChildDirs[i]->isFile()) {
            File::writeAllBytes(fullpath, mChildDirs[i]->mData, mChildDirs[i]->mNode.mDataSize);
        }
    }
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

bool JKRDirectory::isDirectory() {
    return mAttr & JKRFileAttr_FOLDER;
}

bool JKRDirectory::isFile() {
    return mAttr & JKRFileAttr_FILE;
}
