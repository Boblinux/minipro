#ifndef MD_UPLOAD_H
#define MD_UPLOAD_H

#include "TcpServer.h"
#include "md5.h"
#include "Fileproxy.h" 

const int PAGE_SIZE = 4096*256;
std::vector<int> conndfd(3);
enum upStatuFile { upstatu1, upstatu2, upstatu3, upstatu4 };
upStatuFile upStatu = upstatu1;

Md::Fileproxy g_File;
std::string g_FileName;

inline std::string getFileName(std::string FileName)
{
  int i=FileName.size()-1;
  for( ; i>=0; i--)
  {
    if(FileName[i] == '/')
    {
      break;
    }
  }
  return FileName.substr(i+1, FileName.size()-i-1);
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

void initFileNumber(const Reactor::TcpConnectionPtr& conn,
                    Reactor::Buffer* buf)
{ 
  while(buf->readableBytes() >= 4)
  {
    ::send(conndfd[0], "upppload", 8, MSG_NOSIGNAL);
    ::send(conndfd[1], "upppload", 8, MSG_NOSIGNAL);
    ::send(conndfd[2], "upppload", 8, MSG_NOSIGNAL);
    
    
    const void *tmp = buf->peek();
    int32_t be32 = *static_cast<const int32_t*>(tmp);
    size_t fileNameMD5Len = ::ntohl(be32);
    std::cout << fileNameMD5Len << '\n';
    
    if(buf->readableBytes() >= 8+fileNameMD5Len)
    {
      buf->retrieve(4);
      tmp = buf->peek();
      be32 = *static_cast<const int32_t*>(tmp);
      size_t blockNum = ::ntohl(be32);
      buf->retrieve(4);
      std::cout << blockNum << '\n';

      std::string FileNameMD5 = buf->readAsBlock(fileNameMD5Len);
      g_FileName = getFileName(FileNameMD5.substr(0,fileNameMD5Len-32));
      
      if(!g_File.lsFile(g_FileName))
      {
        g_File.addFile(g_FileName, blockNum, FileNameMD5.substr(fileNameMD5Len-32));
        int fileNameLen = g_FileName.size();
        uint32_t nbe = htonl(fileNameLen);
        ::send(conndfd[0], &nbe, 4, MSG_NOSIGNAL); //send fileNameLen
        ::send(conndfd[1], &nbe, 4, MSG_NOSIGNAL); //send fileNameLen
        ::send(conndfd[2], &nbe, 4, MSG_NOSIGNAL); //send fileNameLen

        ::send(conndfd[0], g_FileName.data(), fileNameLen, MSG_NOSIGNAL);
        ::send(conndfd[1], g_FileName.data(), fileNameLen, MSG_NOSIGNAL);
        ::send(conndfd[2], g_FileName.data(), fileNameLen, MSG_NOSIGNAL);

        std::cout << g_FileName << '\n';
        conn->send("ACK");
        upStatu = upstatu2;
      }
      else
      {
        if(g_File.lsFileFinish(g_FileName))
        {
          upStatu = upstatu4;
          conn->send("success");
        }
        else
        {
          std::string MD5string = g_File.getFileidString(g_FileName);
          conn->send(MD5string);
        }
        upStatu = upstatu3;
      }

    }
    else
    {
      break;
    }
    
  }
}

void initFileMD5(const Reactor::TcpConnectionPtr& conn,
                 Reactor::Buffer* buf)
{
  if(buf->readableBytes() >= 32*g_File.getFileNum(g_FileName))
  {
    std::string inputstr = buf->readAsBlock(32*g_File.getFileNum(g_FileName));
    
    g_File.setBlockFileMD5(g_FileName, inputstr);
    std::cout << inputstr << '\n';
    upStatu = upstatu3;
  }
}

void sendBlock(const Reactor::TcpConnectionPtr& conn,
               Reactor::Buffer* buf)
{
  while(upStatu == upstatu3 && buf->readableBytes() >= 4)
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
      
      std::string inputstr = buf->readAsBlock(blocksize);
      std::string md5 = getstringMD5(inputstr);
      if(md5 == g_File.getBlockFileMD5(blockid, g_FileName)) 
      {
        int filelocation = convertMD5toInt(md5)%3;
        
        Reactor::Buffer Buf;
        Buf.append(md5.data(), md5.size());
        Buf.append(inputstr.data(), blocksize);
        Buf.prepend(&bsbe32, sizeof bsbe32);
        ::send(conndfd[filelocation], Buf.readAsString().data(), 4+32+blocksize, MSG_NOSIGNAL);

        g_File.setFileuploadId(g_FileName, blockid);
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
    if(g_File.lsFileFinish(g_FileName))
    {
      upStatu = upstatu4;
      conn->send("success");
      g_FileName.clear();
      break;
    }
  }
}

void upLoadFile(const Reactor::TcpConnectionPtr& conn,
                Reactor::Buffer* buf, upStatuFile Status)
{
  switch(Status)
  {
    case upstatu1:  initFileNumber(conn, buf); 
                  break;
    case upstatu2:  initFileMD5(conn, buf); 
                  break;
    case upstatu3:  sendBlock(conn, buf); 
                  break;
    case upstatu4:  break;
  }
}

#endif