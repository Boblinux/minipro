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
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <arpa/inet.h>
#include <thread>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

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

void onConnection(const Reactor::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("new connection [%s] from %s\n", conn->name().c_str(), conn->peerAddress().toHostPort().c_str());
  }
  else
  {
    printf("connection [%s] is down\n", conn->name().c_str());
  }
}
int conter = 0;
void onMessage(const Reactor::TcpConnectionPtr& conn,
               Reactor::Buffer* buf)
{
  while(buf->readableBytes() >= 4)
  {
    const void *tmp2 = buf->peek();
    int32_t bsbe32 = *static_cast<const int32_t*>(tmp2);
    int32_t blocksize = ::ntohl(bsbe32);
    //std::cout << blocksize << '\n';

    if(buf->readableBytes() >= 36+static_cast<size_t>(blocksize))
    {
      buf->retrieve(4);
      std::string blockmd5 = buf->readAsBlock(32);
      //std::cout << blockmd5 << '\n';

      std::string inputstr = buf->readAsBlock(blocksize);
      //std::cout << inputstr << '\n';
      std::string md5 = getstringMD5(inputstr);
      //std::cout << md5 << '\n';
      if(blockmd5 == md5)
      {
        conter++;
        std::cout << conter << '\n';
        std::ofstream f_out;
        std::string filename;

        filename = "./File/" + md5 + ".txt";
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

int main(int argc, char* argv[])
{
  printf("main(): pid = %d\n", getpid());

  Reactor::InetAddress listenAddr(9983);
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
