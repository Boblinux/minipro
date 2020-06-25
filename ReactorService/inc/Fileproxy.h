#ifndef MD_Proxy_fILE_H
#define MD_Proxy_fILE_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>

namespace Md
{

class FileValue
{

public:

FileValue(unsigned long blockNum, std::string md5)
: blockNum_(blockNum),
  fileMD5_(md5)
{
    BlockFileMD5_ = std::vector<std::string>(blockNum);
    uploadId_ = std::vector<bool>(blockNum, false);
    downloadId_ = std::vector<bool>(blockNum, false);
}

void setFileMD5(std::string md5)
{
    fileMD5_ = md5;
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
      if(uploadId_[i] == true) //false->true
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

std::string getFileMD5()
{
    return fileMD5_;
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
    std::string                 fileMD5_;
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

void addFile(std::string FileName, unsigned long blockNum, std::string FileMD5)
{
    FileMap_[FileName] = new FileValue(blockNum, FileMD5);
}
   
std::string getFileidString(std::string FileName)
{
    return FileMap_[FileName]->getuploadId();
}

void setFileMD5(std::string FileName, std::string MD5)
{
    FileMap_[FileName]->setFileMD5(MD5);
}

void setBlockFileMD5(std::string FileName, std::string MD5string)
{
    FileMap_[FileName]->setBlockFileMD5(MD5string);
}

void setFileuploadId(std::string FileName, int id)
{
    FileMap_[FileName]->setuploadId(id);
}

std::string getBlockFileMD5(int id, std::string FileName)
{
    return FileMap_[FileName]->getBlockFileMD5(id);
}

std::string getFileMD5(std::string FileName)
{
    return FileMap_[FileName]->getFileMD5();
}

bool lsFile(std::string FileName)
{
    std::map<std::string, FileValue *>::iterator it;
    it = FileMap_.find(FileName);
    if(it != FileMap_.end())
    {
        return true;
    }
    return false;
}

bool lsFileFinish(std::string FileName)
{
    return FileMap_[FileName]->lsFileFinish();
}

unsigned long getFileNum(std::string FileName)
{
    return FileMap_[FileName]->getFileNum();
}

private:
    std::map<std::string, FileValue *>        FileMap_;
};

}; // namespace MD5






#endif