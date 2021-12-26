#include "JKRArchive.h"
#include <direct.h>
#include <dirent.h>
#include <iostream>
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

void JKRArchive::save(const std::string &filePath, bool reduceStrings) {
    BinaryWriter* writer = new BinaryWriter(filePath, EndianSelect::Big);
    write(*writer, reduceStrings);
    writer->~BinaryWriter();
}

void JKRArchive::unpack(const std::string &filePath) {
    std::string fullpath;
    fullpath = filePath + "/" + mRoot->mName;
    mkdir(fullpath.c_str());
    mRoot->unpack(fullpath);
}

void JKRArchive::importFromFolder(const std::string &filePath) {
    u32 lastSlashIdx = filePath.rfind('\\');
    std::string name = filePath.substr(lastSlashIdx + 1);
    mRoot = new JKRFolderNode();
    mRoot->mIsRoot = true;
    mRoot->mName = name;
    mFolderNodes.push_back(mRoot);
    createDir(".", JKRFileAttr_FOLDER, mRoot, mRoot);
    createDir("..", JKRFileAttr_FOLDER, nullptr, mRoot);

    DIR* dir = opendir(filePath.c_str());
    dirent* ent;

    while ((ent = readdir(dir))) {
        if (!strcmp(ent->d_name, "..") || !strcmp(ent->d_name, "."))
            continue; 

        if (ent->d_type == DT_DIR) {
            createDir(ent->d_name, JKRFileAttr_FOLDER, createFolder(ent->d_name, mRoot), mRoot);
        }
        else if (ent->d_type == DT_REG) {
            std::cout << ent->d_name << '\n';
            JKRDirectory* file = createFile(ent->d_name, mRoot);
            file->mData = File::readAllBytes(filePath + "/" + ent->d_name, &file->mNode.mDataSize);
        }
    }
}

JKRDirectory* JKRArchive::createDir(const std::string &dirName, JKRFileAttr attr, JKRFolderNode *pNode, JKRFolderNode *pParentNode) {
    JKRDirectory* newDir = new JKRDirectory();
    newDir->mName = dirName;
    newDir->mAttr = attr;
    newDir->mFolderNode = pNode;
    newDir->mParentNode = pParentNode;
    pParentNode->mChildDirs.push_back(newDir);
    mDirectories.push_back(newDir);
    return newDir;
}

JKRDirectory* JKRArchive::createFile(const std::string &fileName, JKRFolderNode*pParentNode) {
    validateName(pParentNode, fileName);
    JKRDirectory* newFile = createDir(fileName, (JKRFileAttr)(JKRFileAttr_FILE | JKRFileAttr_LOAD_TO_MRAM), nullptr, pParentNode);
    
    if (!mSyncFileIds) {
        newFile->mNode.mNodeIdx = mNextFileIdx;
        mNextFileIdx++;
    }

    return newFile;
}

JKRFolderNode* JKRArchive::createFolder(const std::string &folderName, JKRFolderNode*pParentNode) {
    validateName(pParentNode, folderName);
    JKRFolderNode* newFolder = new JKRFolderNode();
    newFolder->mName = folderName;
    mFolderNodes.push_back(newFolder);

    createDir(newFolder->mName, JKRFileAttr_FOLDER, newFolder, pParentNode);
    createDir(".", JKRFileAttr_FOLDER, newFolder, newFolder);
    createDir("..", JKRFileAttr_FOLDER, pParentNode, newFolder);
    return newFolder;
}

void JKRArchive::read(BinaryReader &reader) {
    if (reader.readString(0x4) != "RARC") {
        printf("Fatal error! File is not a valid JKRArchive");
        return;
    }

    mHeader = *reinterpret_cast<const JKRArchiveHeader*>(reader.readBytes(sizeof(JKRArchiveHeader)));
    mDataHeader = *reinterpret_cast<const JKRArchiveDataHeader*>(reader.readBytes(sizeof(JKRArchiveDataHeader)));
    mNextFileIdx = reader.read<u16>();
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
        dir->mNameOffs = dir->mNode.mAttrAndNameOffs & 0x00FFFFFF;
        dir->mAttr = (JKRFileAttr)(dir->mNode.mAttrAndNameOffs >> 24);
        dir->mName = reader.readNullTerminatedStringAt(mDataHeader.mStringTableOffset + mHeader.mHeaderSize + dir->mNameOffs);

        if (i < mDataHeader.mFileNodeCount - 1)
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

    for (JKRFolderNode* node : mFolderNodes) {
        for (s32 y = node->mNode.mFirstFileOffs; y < (node->mNode.mFirstFileOffs + node->mNode.mFileCount); y++) {
            JKRDirectory* childDir = mDirectories[y];
            childDir->mParentNode = node;
            node->mChildDirs.push_back(childDir);
        }
    }
}

void JKRArchive::write(BinaryWriter &writer, bool reduceStrings) {
    sortNodesAndDirs();

    s32 dirOffs = 0x40;
    s32 fileOffs = dirOffs + align32(mFolderNodes.size() * 0x10);
    s32 stringOffs = fileOffs + align32(mDirectories.size() * 0x14);

    writer.seek(stringOffs, std::ios::beg);
    StringPool* pool = new StringPool(StringPoolFormat_NULL_TERMINATED);
    pool->write(".");
    pool->write("..");
    mRoot->mNode.mNameOffs = pool->write(mRoot->mName);

    if (reduceStrings) {
        collectStrings(mRoot, pool, reduceStrings);
    }
    else {
        pool->mLookUp = false;
        collectStrings(mRoot, pool, reduceStrings);
    }

    writer.writeBytes(pool->mBuffer.data(), pool->mBuffer.size());
    s32 stringSize = pool->size();
    while ((stringSize % 32) != 0) stringSize++;
    free(pool);

    writer.seek(dirOffs, std::ios::beg);

    for (JKRFolderNode* node : mFolderNodes) {
        writer.writeString(node->getShortName());
        writer.write<u32>(node->mNode.mNameOffs);
        writer.write<u16>(nameHash(node->mName));
        writer.write<u16>(node->mChildDirs.size());
        writer.write<u32>(node->mNode.mFirstFileOffs);
    }

    writer.seek(0x0, std::ios::end);
    writer.align32();
    u32 fileDataOffs = writer.size() - 0x20;

    u32 mramSize;
    u32 aramSize;
    u32 dvdSize;

    writeFileData(writer, mMRAMFiles, &mramSize);
    writeFileData(writer, mARAMFiles, &aramSize);
    writeFileData(writer, mDVDFiles, &dvdSize);

    u32 fileDataSize = mramSize + aramSize + dvdSize;

    writer.seek(fileOffs, std::ios::beg);

    for (JKRDirectory* dir : mDirectories) {
        writer.write<u16>(dir->mNode.mNodeIdx);
        writer.write<u16>(nameHash(dir->mName));
        writer.write<u32>((dir->mAttr << 24) | dir->mNameOffs);
        writer.write<u32>(dir->mNode.mData);
        writer.write<u32>(dir->mNode.mDataSize);
        writer.writePadding(0x0, 4);
    }

    u32 fileSize = writer.size();
    writer.seek(0x0, std::ios::beg);

    writer.writeString("RARC");
    writer.write<u32>(fileSize);
    writer.write<u32>(0x20);
    writer.write<u32>(fileDataOffs);
    writer.write<u32>(fileDataSize);
    writer.write<u32>(mramSize);
    writer.write<u32>(aramSize);
    writer.write<u32>(dvdSize);

    writer.write<u32>(mFolderNodes.size());
    writer.write<u32>(0x20);
    writer.write<u32>(mDirectories.size());
    writer.write<u32>(fileOffs - 0x20);
    writer.write<u32>(stringSize);
    writer.write<u32>(stringOffs - 0x20);
    writer.write<u16>(mNextFileIdx);
    writer.write<u8>(mSyncFileIds);
}

void JKRArchive::writeFileData(BinaryWriter &writer, std::vector<JKRDirectory*> files, u32 *pSize) {
    u32 startPos = writer.size();

    for (JKRDirectory* dir : files) {
        writer.writeBytes(dir->mData, dir->mNode.mDataSize);
        writer.align32();
    }
    u32 out = writer.size() - startPos;
    *pSize = out;
}

void JKRArchive::sortNodeAndDirs(JKRFolderNode*pNode) {
    std::vector<JKRDirectory*> shortcuts;
    for (s32 i = 0; i < pNode->mChildDirs.size(); i++) {
        if (pNode->mChildDirs[i]->isShortcut())    
            shortcuts.push_back(pNode->mChildDirs[i]);
    }

    for (JKRDirectory* dir : shortcuts) {
        pNode->mChildDirs.erase(pNode->mChildDirs.begin() + Util::getVectorIndex(pNode->mChildDirs, dir));
        pNode->mChildDirs.push_back(dir);
    }

    pNode->mNode.mFirstFileOffs = mDirectories.size();
    pNode->mNode.mFileCount = pNode->mChildDirs.size();

    for (s32 i = 0; i < pNode->mChildDirs.size(); i++)
        mDirectories.push_back(pNode->mChildDirs[i]);

    for (JKRDirectory* dir : pNode->mChildDirs) {
        if (dir->isDirectory() && !dir->isShortcut())
            sortNodeAndDirs(dir->mFolderNode);
    }
}

void JKRArchive::sortNodesAndDirs() {
    mDirectories.clear();
    sortNodeAndDirs(mRoot);

    if (mSyncFileIds)
        mNextFileIdx = mDirectories.size();

    for (JKRDirectory* dir : mDirectories) {
        if (dir->isDirectory())  {
            if (dir->mFolderNode)
                dir->mNode.mData = Util::getVectorIndex(mFolderNodes, dir->mFolderNode);
            else    
                dir->mNode.mData = 0xFFFFFFFF;
        }
        else {
            if (mSyncFileIds)
                dir->mNode.mNodeIdx = Util::getVectorIndex(mDirectories, dir);

            if (dir->getPreloadType() == JKRPreloadType_MRAM)
                mMRAMFiles.push_back(dir);
            else if (dir->getPreloadType() == JKRPreloadType_ARAM)
                mARAMFiles.push_back(dir);
            else if (dir->getPreloadType() == JKRPreloadType_DVD)
                mDVDFiles.push_back(dir);
        }
    }
}

bool JKRArchive::validateName(JKRFolderNode*pNode, const std::string &fileName) {
    for (s32 i = 0; i < mDirectories.size(); i++) {
        if (!mDirectories[i]->mName.compare(pNode->mName)) {
            printf("Folder name already exists!\n");
            return false;
        }
    }

    return true;
}

void JKRArchive::collectStrings(JKRFolderNode*pNode, StringPool*pPool, bool reduceStrings) {
    if (reduceStrings) {
        for (s32 i = 0; i < pNode->mChildDirs.size(); i++) {
            pNode->mChildDirs[i]->mNameOffs = pPool->write(pNode->mChildDirs[i]->mName);

            if (pNode->mChildDirs[i]->isDirectory() && !pNode->mChildDirs[i]->isShortcut()) { 
                pNode->mChildDirs[i]->mFolderNode->mNode.mNameOffs = pNode->mChildDirs[i]->mNameOffs;
                collectStrings(pNode->mChildDirs[i]->mFolderNode, pPool, reduceStrings);  
            }
        }
    }
    else {
        for (s32 i = 0; i < pNode->mChildDirs.size(); i++) {
            if (pNode->mChildDirs[i]->isShortcut()) 
                pNode->mChildDirs[i]->mNameOffs = pPool->find(pNode->mChildDirs[i]->mName);
            else 
                pNode->mChildDirs[i]->mNameOffs = pPool->write(pNode->mChildDirs[i]->mName);

            if (pNode->mChildDirs[i]->isDirectory() && !pNode->mChildDirs[i]->isShortcut()) {
                pNode->mChildDirs[i]->mFolderNode->mNode.mNameOffs = pNode->mChildDirs[i]->mNameOffs;
                collectStrings(pNode->mChildDirs[i]->mFolderNode, pPool, reduceStrings);  
            }
        }
    }
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

std::string JKRFolderNode::getShortName() {
    std::string ret = mName;
    
    if (mIsRoot) 
        return "ROOT";

    if (ret.size() < 4) {
        while (ret.size() < 4) {
            ret.push_back(' ');
        }
    }
    else
        ret = mName.substr(0, 4);

    std::transform(ret.begin(), ret.end(), ret.begin(), [](u8 c){ return std::toupper(c); });
    return ret;
}

u16 JKRArchive::nameHash(const std::string &str) {
    u16 ret = 0;
    for (s32 i = 0; i < str.size(); i++) {
        ret *= 0x3;
        ret += (u16)str[i];
    }

    return ret;
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