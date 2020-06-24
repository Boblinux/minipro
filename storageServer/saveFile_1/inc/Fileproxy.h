#ifndef MD_Proxy_fILE_H
#define MD_Proxy_fILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>
//#include "MD5.h"

namespace Md
{

class FileValue
{

public:

FileValue(unsigned long blockNum)
: blockNum_(blockNum)
{
    BlockFileMD5_ = std::vector<std::string>(blockNum);
    uploadId_ = std::vector<bool>(blockNum, false);
    downloadId_ = std::vector<bool>(blockNum, false);
}

void setBlockFileMD5(std::string md5)
{
    for(int i=0; i<(int)blockNum_; i++)
    {
        BlockFileMD5_[i] = md5.substr(i*32,32);
    }
}

void setuploadId(int id)
{
    uploadId_[id] = true;
}

void setdownloadId(int id)
{
    downloadId_[id] = true;
}

std::string getuploadId()
{
    std::string idstr;
    for(int i=0; i<(int)blockNum_; i++)
    {
      if(uploadId_[i] == false)
      {
        idstr += std::to_string(i) + ':';
      }
    }
    return idstr;
}

std::string getBlockFileMD5(int id)
{
    return BlockFileMD5_[id];
}

unsigned long getFileNum()
{
    return blockNum_;
}

bool lsFileFinish()
{
    for(int i=0; i<static_cast<int>(blockNum_); i++)
    {
        if(uploadId_[i] == false)
        {
            return false;
        }
    }
    return true;
}

~FileValue()
{
    BlockFileMD5_.clear();
    uploadId_.clear();
    downloadId_.clear();
}

private:
    unsigned long               blockNum_;
    std::vector<std::string>    BlockFileMD5_;
    std::vector<bool>           uploadId_; 
    std::vector<bool>           downloadId_;  
};


class Fileproxy
{

public:

Fileproxy() 
{  
}

~Fileproxy() 
{
}

void addFile(unsigned long blockNum, std::string FileMD5)
{
    FileMap_[FileMD5] = new FileValue(blockNum);
}
   
std::string getFileidString(std::string FileMD5)
{
    return FileMap_[FileMD5]->getuploadId();
}

void setFileMD5(std::string FileMD5, std::string MD5string)
{
    FileMap_[FileMD5]->setBlockFileMD5(MD5string);
}

void setFileuploadId(std::string FileMD5, int id)
{
    FileMap_[FileMD5]->setuploadId(id);
}

std::string getBlockFileMD5(int id, std::string FileMD5)
{
    return FileMap_[FileMD5]->getBlockFileMD5(id);
}

bool lsFile(std::string FileMD5)
{
    std::map<std::string, FileValue *>::iterator it;
    it = FileMap_.find(FileMD5);
    if(it != FileMap_.end())
    {
        return true;
    }
    return false;
}

bool lsFileFinish(std::string FileMD5)
{
    return FileMap_[FileMD5]->lsFileFinish();
}

unsigned long getFileNum(std::string FileMD5)
{
    return FileMap_[FileMD5]->getFileNum();
}

private:
    std::map<std::string, FileValue *>        FileMap_;
};

}; // namespace MD5






#endif