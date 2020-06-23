#include "TcpServer.h"
#include "Loop.h"
#include "InetAddress.h"
#include "MD5.h"
#include "Fileproxy.h" 
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

const int PAGE_SIZE = 4096*256;
enum StateFile { initFileNum, initFileMD5, sendBlock, sendEnd, sendFinish, GG };
StateFile State = initFileNum;

Md::Fileproxy g_File;
std::string g_FileMD5;

inline std::string getstringMD5(std::string str)
{
  unsigned char tmp1[MD5_DIGEST_LENGTH];
  ::MD5((unsigned char*) str.data(), str.size(), tmp1);
  char buf[33] = {0};  
  char tmp2[3] = {0};  
  for(int i = 0; i < MD5_DIGEST_LENGTH; i++ )  
  {  
      sprintf(tmp2,"%02X", tmp1[i]);  
      strcat(buf, tmp2);  
  }
  std::string md5 = buf;
  return md5;
}

inline int convertMD5toInt(std::string md5)
{
  int res = 0;
  for(int i=0; i<32; i++)
  {  
    res += static_cast<int>(md5[i]);
  }
  return res;
}

void onConnection(const Reactor::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("new connection [%s] from %s\n", conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
    State = initFileNum;
  }
  else
  {
    printf("connection [%s] is down\n", conn->name().c_str());
  }
}



void onMessage(const Reactor::TcpConnectionPtr& conn,
               Reactor::Buffer* buf)
{
  std::string inputstr;
  if(State == initFileNum && buf->readableBytes() >= 36)
  {
    const void *tmp = buf->peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    int blockNum = ::ntohl(be32);
    buf->retrieve(4);
    std::cout << blockNum << '\n';
    
    std::string FileMD5 = buf->readAsBlock(32);
    std::cout << FileMD5 << '\n';
    g_FileMD5 = FileMD5;
    
    if(!g_File.lsFile(FileMD5))
    {
      g_File.addFile(blockNum, FileMD5);
      conn->send("ACK");
      State = initFileMD5;
    }
    else
    {
      if(g_File.lsFileFinish(g_FileMD5))
      {
        State = sendEnd;
        conn->send("success");
      }
      else
      {
        std::string MD5string = g_File.getFileidString(FileMD5);
        conn->send(MD5string);
      }
      State = sendBlock;
    }
  }

  if(State == initFileMD5 && buf->readableBytes() >= 32*g_File.getFileNum(g_FileMD5))
  {
    inputstr = buf->readAsBlock(32*g_File.getFileNum(g_FileMD5));
    std::cout << inputstr << '\n';
    g_File.setFileMD5(g_FileMD5, inputstr);
    State = sendBlock;
  }
  
  while(State == sendBlock && buf->readableBytes() >= 4)
  {
    const void *tmp2 = buf->peek();
    int32_t bsbe32 = *static_cast<const int32_t*>(tmp2);
    int32_t blocksize = ::ntohl(bsbe32);
    //std::cout << blocksize << '\n';
    
    if(buf->readableBytes() >= 8+static_cast<size_t>(blocksize))
    {
      buf->retrieve(4);
      const void *tmp1 = buf->peek();
      int32_t idbe32 = *static_cast<const int32_t*>(tmp1);
      int32_t blockid = ::ntohl(idbe32);
      buf->retrieve(4);
      std::cout << blockid << '\n';
      
      inputstr = buf->readAsBlock(blocksize);
      std::string md5 = getstringMD5(inputstr);
      if(md5 == g_File.getBlockFileMD5(blockid, g_FileMD5)) 
      {
        int filelocation = convertMD5toInt(md5)%3;
        std::ofstream f_out;
        std::string filename;
        if(filelocation == 0)
        {
          filename = "../saveFile_0/" + md5 + ".txt";
        }
        else if(filelocation == 1)
        {
          filename = "../saveFile_1/" + md5 + ".txt";
        }
        else
        {
          filename = "../saveFile_2/" + md5 + ".txt";
        }
        
        f_out.open(filename, std::ios::out | std::ios::app); 
        f_out << inputstr;
        f_out.close();
        
        g_File.setFileuploadId(g_FileMD5, blockid);
        std::cout << "seccess" <<std::endl;
      }
      else
      {
        std::cout << "error" <<std::endl;
      }
    }
    else
    {
      break;
    }
    if(g_File.lsFileFinish(g_FileMD5))
    {
      State = sendEnd;
      conn->send("success");
      break;
    }
  }
  
}

int main(int argc, char* argv[])
{
  printf("main(): pid = %d\n", getpid());

  Reactor::InetAddress listenAddr(9981);
  Reactor::Loop loop;
  Reactor::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  if (argc > 1) {
    server.setThreadNum(atoi(argv[1]));
  } 
  server.start();
  loop.loop();
}
