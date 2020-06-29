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

#endif