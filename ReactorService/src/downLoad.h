#include "TcpServer.h"
#include "Loop.h"
#include "InetAddress.h"
#include "MD5.h"
#include "Fileproxy.h" 
#include "mysocket.h" 
#include "upLoad.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <map>
#include <mutex>
#include <vector>
#include <fstream>
#include <iostream>
#include <openssl/md5.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <arpa/inet.h>
#include <thread>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <sys/wait.h>   
#include <sys/epoll.h>  
#include "upLoad.h"
 

enum downStatuFile { downstatu1, downstatu2, downstatu3, downstatu4 };

void senddownload(int conndfd, downStatuFile &downStatus)
{
  ::send(conndfd, "download", 8, MSG_NOSIGNAL);
  downStatus = downstatu2;
}

void sendFileName(int conndfd, downStatuFile &downStatus, Reactor::Buffer &buf)
{
  if(buf.readableBytes() >= 8)
  {
    std::string str = buf.readAsBlock(8);
    std::cout << str << '\n';
    /*
    std::cout << g_FileName << '\n';
    int len = g_FileName.size();
    uint32_t bsbe = htonl(len);
    ::send(conndfd, &bsbe, 4, MSG_NOSIGNAL);

    if(::send(conndfd, g_FileName.data(), len, MSG_NOSIGNAL) == len)
    {
      downStatus = downstatu3;
    }*/
    //test
    uint32_t bsbe = htonl(11);
    ::send(conndfd, &bsbe, 4, MSG_NOSIGNAL);
    if(::send(conndfd, "kvmap_2.txt", 11, MSG_NOSIGNAL) == 11)
    {
      downStatus = downstatu3;
    }
  }
  
}
Reactor::Buffer* Bloc = new Reactor::Buffer();
void resvBlockFile(int conndfd, downStatuFile &downStatus, Reactor::Buffer &buf, const Reactor::TcpConnectionPtr& conn)
{
  while(buf.readableBytes() >= 4)
  {
    const void *tmp = buf.peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    size_t BlockFileSize = ::ntohl(be32);
    
    if(buf.readableBytes() >= 4+36+BlockFileSize)
    {
      std::cout << BlockFileSize << '\n';
      buf.retrieve(4);
      std::string FileContent = buf.readAsBlock(36+BlockFileSize);

      if(g_File.lsFile("kvmap_2.txt"))
      {
        std::cout << "kvmap_2.txt is in map" << '\n';
        //g_File.saveDownloadBlock("kvmap_2.txt", \
                                  g_File.getidFromMD5("kvmap_2.txt", FileContent.substr(0,32)), \
                                  FileContent.substr(36));
        if(getstringMD5(FileContent.substr(36)) == FileContent.substr(0,32))
        {
          std::cout << "success" << '\n';
          std::cout << getstringMD5(FileContent.substr(36)) << '\n';
          std::cout << FileContent.substr(0,32) << '\n';
          Bloc->append(FileContent.substr(36).data(), FileContent.substr(36).size());
          uint32_t bsbe = htonl(FileContent.substr(36).size());
          Bloc->prepend(&bsbe, 4);
          conn->send(Bloc->readAsString());
        }
      
      }

    }
    else
    {
      break;
    }
    
  }
  
}

void call_backfunc(int conndfd, downStatuFile &downStatus, Reactor::Buffer &buf, const Reactor::TcpConnectionPtr& conn)
{
  if(buf.readableBytes() > 0)
  {
    switch(downStatus)
    {
      case downstatu1:  senddownload(conndfd, downStatus); 
                        break;
      case downstatu2:  sendFileName(conndfd, downStatus, buf);
                        break;
      case downstatu3:  resvBlockFile(conndfd, downStatus, buf, conn); 
                        break;
      case downstatu4:  break;
    }
  }
}

void downLoadFileFromStorage(int conndfd, downStatuFile &downStatus, const Reactor::TcpConnectionPtr& conn)
{
  Reactor::Buffer Buf;
  struct epoll_event cev, cevents[1];  
  int epfd = epoll_create(16);
  cev.events = EPOLLIN;  
  cev.data.fd = conndfd;  
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, conndfd, &cev) == -1) {  
    perror("epoll_ctl: listen_sock");  
    exit(EXIT_FAILURE);  
  }  
  
  senddownload(conndfd, downStatus);
  
  while(true)
  {
    
    int nfds = epoll_wait(epfd, cevents, 16, -1);  
    if (nfds == -1) {  
      perror("epoll_pwait");  
      exit(EXIT_FAILURE);  
    }  

    if (cevents[0].events & EPOLLIN) {  
      Buf.readFd(conndfd);

      call_backfunc(conndfd, downStatus, Buf, conn);

      cev.events = cevents[0].events | EPOLLOUT;  
      if (epoll_ctl(epfd, EPOLL_CTL_MOD, conndfd, &cev) == -1) {  
        perror("epoll_ctl: mod");  
      }  
    }  
  }
}

downStatuFile downStatuTH0 = downstatu1;
downStatuFile downStatuTH1 = downstatu1;
downStatuFile downStatuTH2 = downstatu1;
boost::thread threadFunc[3];
void downLoad(const Reactor::TcpConnectionPtr& conn)
{
  threadFunc[0] = boost::thread(boost::bind(downLoadFileFromStorage, conndfd[0], boost::ref(downStatuTH0), boost::ref(conn)));
  threadFunc[1] = boost::thread(boost::bind(downLoadFileFromStorage, conndfd[1], boost::ref(downStatuTH1), boost::ref(conn)));
  threadFunc[2] = boost::thread(boost::bind(downLoadFileFromStorage, conndfd[2], boost::ref(downStatuTH2), boost::ref(conn))); 
  threadFunc[0].detach();
  threadFunc[1].detach();
  threadFunc[2].detach();
  std::cout << "++++" << '\n';

}
