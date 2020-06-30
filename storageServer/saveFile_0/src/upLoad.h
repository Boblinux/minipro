#include "TcpServer.h"
#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>    
#include <fstream>
#include <iostream>


std::string g_upfileName = "./File/";
enum StatuFile { statu1, statu2, statu3, statu4 };
StatuFile statu = statu1;

void mkfile(const Reactor::TcpConnectionPtr& conn,
            Reactor::Buffer* buf)
{
  while(buf->readableBytes() >= 4)
  {
    const void *tmp = buf->peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    size_t fileLen = ::ntohl(be32);

    if(buf->readableBytes() >= 4+fileLen)
    {
      buf->retrieve(4);
      g_upfileName += buf->readAsBlock(fileLen) + '/';
      mkdir(g_upfileName.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
      statu = statu2;
    }
    else
    {
      break;
    }
  }
}
int conter = 0;
void saveBlockFile(const Reactor::TcpConnectionPtr& conn,
                   Reactor::Buffer* buf)
{
  while(buf->readableBytes() >= 4)
  {
    const void *tmp2 = buf->peek();
    int32_t bsbe32 = *static_cast<const int32_t*>(tmp2);
    int32_t blocksize = ::ntohl(bsbe32);
    if(buf->readableBytes() >= 36+static_cast<size_t>(blocksize))
    {
      buf->retrieve(4);
      std::string blockmd5 = buf->readAsBlock(32);
      std::string inputstr = buf->readAsBlock(blocksize);
      std::string md5 = getstringMD5(inputstr);
      if(blockmd5 == md5)
      {
        conter++;
        std::cout << conter << '\n';
        std::ofstream f_out;
        std::string filename;

        filename = g_upfileName + md5 + ".txt";
        f_out.open(filename, std::ios::out | std::ios::app); 
        f_out << inputstr;
        f_out.close();
      }

    }
    else
    {
      break;
    }
  }
}

void upLoad(const Reactor::TcpConnectionPtr& conn,
            Reactor::Buffer* buf)
{
  switch (statu)
  {
  case statu1 : mkfile(conn, buf);
                break;
  case statu2 : saveBlockFile(conn, buf);
                break;
  
  default:
    break;
  }
}