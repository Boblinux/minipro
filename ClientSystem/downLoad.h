#ifndef MD_DOWNLOAD_H
#define MD_DOWNLOAD_H
#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <map>
#include <thread>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include "File.h"
#include "Buffer.h"
#include "md5.h"
#include <sys/wait.h>   
#include <sys/epoll.h>

std::vector<std::string>  BlockContent;
enum downStatuFile { downstatu1, downstatu2, downstatu3, downstatu4 };
downStatuFile downStatus = downstatu1;
std::map<std::string, int> idMD5;

void recvMD5Block(int conndfd, Reactor::Buffer &buf)
{
  while(buf.readableBytes() >= 4)
  {
    const void *tmp = buf.peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    size_t BlockSize = ::ntohl(be32);
   
    if(buf.readableBytes() >= 4+BlockSize)
    {
      buf.retrieve(4);
      std::cout << BlockSize << '\n';
      std::string readContent = buf.readAsBlock(BlockSize);
      
      std::cout << readContent.substr(0, 20) << '\n';
      //std::string md5 = readContent.substr(0,32);
      //BlockContent[idMD5[md5]] = readContent.substr(0,36);
      //std::cout << BlockContent[idMD5[md5]] << '\n';
    }
    else
    {
      break;
    }
  }
}

void recvidMD5(int conndfd, Reactor::Buffer &buf)
{
  while(buf.readableBytes() >= 4)
  {
    const void *tmp = buf.peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    size_t id = ::ntohl(be32);
    std::cout << id << '\n';
    
    if(buf.readableBytes() >= 36)
    {
      buf.retrieve(4);
      std::string MD5 = buf.readAsBlock(32);
      std::cout << MD5 << '\n';
      if(MD5 == "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG")
      {
        BlockContent = std::vector<std::string>(idMD5.size());
        downStatus = downstatu2;
        break;
      }
      else
      {
        idMD5[MD5] = id;
      }
    }
    else
    {
      break;
    }
  }
} 

void call_backfunc(int& conndfd, int& fd, Reactor::Buffer &buf)
{
  if(buf.readableBytes() > 0)
  {
    switch(downStatus)
    {
      case downstatu1:  recvidMD5(conndfd, buf); 
                        break;
      case downstatu2:  recvMD5Block(conndfd, buf); 
                        break;
      case downstatu3:  //resvBlockFile(conndfd, downStatus, buf); 
                        break;
      case downstatu4:  break;
    }
  }
}

Reactor::Buffer Buf;
void downLoadFile(int conndfd, int fd)
{
  struct epoll_event cev, cevents[16];  
  int epfd = epoll_create(16);
  cev.events = EPOLLIN;  
  cev.data.fd = conndfd;  
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, conndfd, &cev) == -1) {  
    perror("epoll_ctl: listen_sock");  
    exit(EXIT_FAILURE);  
  }  
  sleep(1);
  ::send(conndfd, "1", 1, MSG_NOSIGNAL); //send blockSize

  while(true)
  {
    int nfds = epoll_wait(epfd, cevents, 16, -1);  
    for(int i=0; i<nfds; ++i)
    {
      if (cevents[i].events & EPOLLIN) 
      {  
        Buf.readFd(conndfd);
        //call_backfunc(conndfd, fd, Buf);
        int n = Buf.readableBytes();
        int nn = ::write(fd, Buf.readAsString().data(), n);
        std::cout<<nn<<'\n';
        //recvMD5Block(conndfd, Buf);

        cev.events = cevents[i].events | EPOLLOUT;  
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, conndfd, &cev) == -1)
        {  
          perror("epoll_ctl: mod");  
        }  
      }
    } 
  }   
}

void downLoad(int conndfd, std::string argv)
{
  int fd = open(argv.data(), O_RDWR|O_CREAT);
  if (-1 == fd)
  {   
    printf("Create file Error\n");
  }   
  downStatus = downstatu1;
  downLoadFile(conndfd, fd);
}

void downLoadMD5(int conndfd, std::vector<std::string>& MD5str, std::string argv)
{
  int n, location=0;
  const int BUFFSIZE = 1024*1024;
  char BUFFER[BUFFSIZE];
  ::read(conndfd, BUFFER, 8);
  std::cout <<  BUFFER << '\n';
  
  Reactor::Buffer Buff;
  uint32_t bsbe = htonl(argv.size());
  Buff.append(argv.data(), argv.size());
  Buff.prepend(&bsbe, 4);
  ::send(conndfd, Buff.readAsString().data(), 4+argv.size(), MSG_NOSIGNAL); 

  //::send(conndfd, &bsbe, 4, MSG_NOSIGNAL); //send blockSize
  //::send(conndfd, argv.data(), argv.size(), MSG_NOSIGNAL); //send blockSize
  bzero(BUFFER, BUFFSIZE);
  std::string str;
  if((n = ::read(conndfd, BUFFER, BUFFSIZE)) >= 4)
  {
    std::string str = BUFFER;
    for(int i=0; i+32 <= str.size(); i += 32)
    {
      MD5str.push_back(str.substr(i,32));
    }    
  }
}

void downLoadFileBlock(int conndfd, std::vector<std::string>& MD5str, std::string argv)
{
  int n, location=0;
  const int BUFFSIZE = 1024*1024;
  char BUFFER[BUFFSIZE];
  ::read(conndfd, BUFFER, 8);
  std::cout <<  BUFFER << '\n';
  
  Reactor::Buffer Buff;
  uint32_t bsbe = htonl(argv.size());
  Buff.append(argv.data(), argv.size());
  Buff.prepend(&bsbe, 4);
  ::send(conndfd, Buff.readAsString().data(), 4+argv.size(), MSG_NOSIGNAL); 
  bzero(BUFFER, BUFFSIZE);
  std::string str;
  while(true)
  {
    Buff.readFd(conndfd);
    while(Buff.readableBytes() >= 4)
    {
      const void *tmp = Buff.peek();
      int32_t be32 = *static_cast<const int32_t*>(tmp);
      size_t BlockFileSize = ::ntohl(be32);
      
      if(Buff.readableBytes() >= 4+36+BlockFileSize)
      {
        std::cout << BlockFileSize << '\n';
        Buff.retrieve(4);
        std::string FileContent = Buff.readAsBlock(36+BlockFileSize);
        if(getstringMD5(FileContent.substr(36)) == FileContent.substr(0,32))
        {
          std::cout << "success" << '\n';
          std::string file = "./File/" + FileContent.substr(0,36);
          int fd = open(file.data(), O_RDWR|O_CREAT);
          ::write(fd, FileContent.substr(36).data(), FileContent.substr(36).size());
        }
      }
      else
      {
        break;
      }
    }
  }
}

#endif