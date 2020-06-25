#ifndef MD_fILE_H
#define MD_fILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "MD5.h"
#include "BlockFile.h"

const int PAGE_SIZE = 4096*256;

namespace Md
{

class BlockFile;
class Md5;
class File
{

public:

File(int fd, int conn, unsigned long fileSize) 
    : fd_(fd),
      conn_(conn),
      fileSize_(fileSize),
      md5_(fd_, fileSize_, 0),
      blockNum_((unsigned long)(fileSize_ % PAGE_SIZE == 0 ? fileSize_/PAGE_SIZE:fileSize_/PAGE_SIZE+1)),
      uploadId_(std::vector<bool>(blockNum_, false)),
      downloadId_(std::vector<bool>(blockNum_, false))
{
    initFile();
}
~File() 
{
    BlockFileList_.clear();
}
    
std::string getFileMD5()
{
    return md5_.getMD5();
}

unsigned long getFileBlockNum() {return blockNum_; }

std::vector<BlockFile*> getFileBlockFile() {return BlockFileList_; }

void initFile()
{
    for(int i=0; i<blockNum_; i++)
    {
        if(i != blockNum_-1)
        {
            BlockFile *block = new BlockFile(fd_, conn_, PAGE_SIZE, i, i*PAGE_SIZE);
            BlockFileList_.push_back(block);
        }
        else 
        {
            BlockFile *block = new BlockFile(fd_, conn_, fileSize_%PAGE_SIZE, i, i*PAGE_SIZE);
            BlockFileList_.push_back(block);
        }
    }
}

void sendFileinfo(std::string fileName)
{
    fileName += md5_.getMD5();
    int fileNameMd5Len = fileName.size();
    uint32_t be = htonl(fileNameMd5Len);
    ::send(conn_, &be, 4, MSG_NOSIGNAL); //send fileNameMd5Len
    
    uint32_t nbe = htonl(blockNum_);
    write(conn_, &nbe, 4); //send blockNum

    ::send(conn_, fileName.data(), fileNameMd5Len, MSG_NOSIGNAL);  //send fileNameMd5
    //write(conn_, md5_.getMD5().data(), 32);
}

void sendFileBlockMD5info()
{
    for(int i=0; i<blockNum_; i++)
    {
        BlockFileList_[i]->sendBlockMD5();
    }
}

void sendFileBlock()
{
    for(int i=0; i<blockNum_; i++)
    {
        if(uploadId_[i] == false)
        {
            BlockFileList_[i]->sendBlockFile();
        }
    }
}

void updateuploadId(std::string idstr)
{
    std::string tmp;
    int start = 0, end = 0;
    for(int i=0; i<(int)idstr.size(); i++)
    {
        if(idstr[i] == ':')
        {
            tmp = idstr.substr(start, i-start);
            uploadId_[atoi(tmp.data())] = true;
            start = i+1;
        }
    }
}

void updatedownloadId(std::string idstr)
{
    std::string tmp;
    int start = 0, end = 0;
    for(int i=0; i<(int)idstr.size(); i++)
    {
        if(idstr[i] == ':')
        {
            tmp = idstr.substr(start, i-start);
            downloadId_[atoi(tmp.data())] = true;
            start = i+1;
        }
    }
}


private:
    int                         fd_; 
    int                         conn_; 
    unsigned long               fileSize_;
    Md5                         md5_; 
    unsigned long               blockNum_;
    std::vector<BlockFile*>     BlockFileList_;
    std::vector<bool>           uploadId_; 
    std::vector<bool>           downloadId_;  
};

}; // namespace MD5

#endif