#include "TcpServer.h"
#include <sys/stat.h>    
#include <fcntl.h>
#include <dirent.h>
#include "md5.h"
#include <string.h>

std::string g_downfileName = "./File/";

void readFileList(const Reactor::TcpConnectionPtr& conn, const char *basePath)
{
  DIR *dir;
  struct dirent *ptr;

  if ((dir=opendir(basePath)) == NULL)
  {
    perror("Open dir error...");
    exit(1);
  }

  while ((ptr=readdir(dir)) != NULL)
  {
    if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //current dir OR parrent dir
      continue;
    else if(ptr->d_type == 8)    //file
    {
      std::string Name = ptr->d_name;
      std::string fileName = g_downfileName + Name;
      int fd = open(fileName.data(),O_RDONLY);
      if(-1 == fd)
      {
        perror("ferror");
        exit(-1);
      }
      Reactor::Buffer sendbuf;
      sendbuf.append(Name.data(), Name.size());
      
      struct stat fs;
      fstat(fd, &fs);
      uint32_t bsbe = htonl(fs.st_size);
      sendbuf.prepend(&bsbe, sizeof bsbe);

      int n;
      char Buffer[1024*1024];
      bzero(Buffer, 1024*1024);
      if((n = read(fd, Buffer, 1024*1024)) == fs.st_size);
      {
        sendbuf.append(static_cast<const char *>(Buffer), n);
        conn->send(sendbuf.readAsString());
        printf("d_name:%s%s\n",basePath,ptr->d_name);
      }
    }
  }
  closedir(dir);
}

void downLoad(const Reactor::TcpConnectionPtr& conn,
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
      g_downfileName += buf->readAsBlock(fileLen) + '/';
      std::cout << g_downfileName << '\n';
      readFileList(conn, g_downfileName.data());
      g_downfileName.clear();
      g_downfileName = "./File/";
    }
    else
    {
      break;
    }
  }
  
}

