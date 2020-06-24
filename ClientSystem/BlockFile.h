#ifndef MD_BLOCKFILE_H
#define MD_BLOCKFILE_H

#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include "MD5.h"

namespace Md
{

class BlockFile
{

public:

BlockFile(int fd, int conn, unsigned long blockSize, int id, off_t offset) 
    : fd_(fd),
      conn_(conn),
      blockSize_(blockSize),
      id_(id),
      offset_(offset),
      md5_(fd_, blockSize_, offset_)
{
    
}
~BlockFile() {}
    
unsigned long getBlockSize()
{
    return blockSize_;
}

std::string getBlockFileMD5()
{
    return md5_.getMD5();
}

int getBlockFileid()
{
    return id_;
}

off_t getBlockFileOffset()
{
    return offset_;
}

void sendBlockFile()
{
    char *bockBuffer = (char*)mmap(0, blockSize_, PROT_READ, MAP_SHARED, fd_, offset_);
    uint32_t bsbe = htonl(blockSize_);
    ::send(conn_, &bsbe, 4, MSG_NOSIGNAL); //send blockSize
    
    uint32_t idbe = htonl(id_); //send id
    ::send(conn_, &idbe, 4, MSG_NOSIGNAL);

    ::send(conn_, bockBuffer, blockSize_, MSG_NOSIGNAL);   //send block
    munmap(bockBuffer, blockSize_); 
}

void sendBlockMD5()
{
    ::send(conn_, md5_.getMD5().data(), 32, MSG_NOSIGNAL);
}

private:
    int             fd_;
    int             conn_; 
    unsigned long   blockSize_;
    uint32_t        id_;
    off_t           offset_;
    Md5             md5_;
};

}; // namespace MD5

#endif