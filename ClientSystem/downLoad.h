#ifndef MD_DOWNLOAD_H
#define MD_DOWNLOAD_H
#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include <map>
#include <thread>
#include <mutex>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include "File.h"
#include "Buffer.h"
#include "md5.h"
#include <sys/wait.h>   
#include <sys/epoll.h>

std::vector<std::string> MD5str;
std::vector<std::string> blockMD5Num;
std::vector<std::string> content;
std::mutex mux;

void remove_dir(const char *dir)
{
	char cur_dir[] = ".";
	char up_dir[] = "..";
	char dir_name[128];
	DIR *dirp;
	struct dirent *dp;
	struct stat dir_stat;
	if ( 0 > stat(dir, &dir_stat) ) {
		perror("get directory stat error");
	}

	if ( S_ISREG(dir_stat.st_mode) ) {	
		remove(dir);
	} else if ( S_ISDIR(dir_stat.st_mode) ) {	
		dirp = opendir(dir);
		while ( (dp=readdir(dirp)) != NULL ) {
			if ( (0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name)) ) {
				continue;
			}
			
			sprintf(dir_name, "%s/%s", dir, dp->d_name);
			remove_dir(dir_name);  
		}
		closedir(dirp);

		rmdir(dir);	
	} else {
		perror("unknow file type!");	
	}
}

void combinateFile(std::vector<std::string> MD5str, const char *basePath, const char *agrv)
{
  DIR *dir;
  struct dirent *ptr;
  if ((dir=opendir(basePath)) == NULL)
  {
    perror("Open dir error...");
    exit(1);
  }
  std::string fileName = basePath;
  while ((ptr=readdir(dir)) != NULL)
  {
    if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    //current dir OR parrent dir
      continue;
    else if(ptr->d_type == 8)    //file
    {
      std::string Name = ptr->d_name;
      
      std::string file = "./" + fileName +'/'+Name;
      int fd = open(file.data(),O_RDONLY);
      if(-1 == fd)
      {
        perror("ferror");
        exit(-1);
      }
      struct stat fs;
      fstat(fd, &fs);
      int n;
      char Buffer[1024*1024];
      bzero(Buffer, 1024*1024);
      std::string tmp = Name.substr(0,32);
      if((n = read(fd, Buffer, 1024*1024)) == fs.st_size);
      {
        std::string buf(Buffer, Buffer+n);
        tmp += buf;
      }
      content.push_back(tmp);
    }
  }
  closedir(dir);
  std::string file = "./" + fileName;
  remove_dir(file.data());

  if(content.size() == MD5str.size())
  {
    int fd = ::open(agrv, O_RDWR | O_CREAT);
    for(int i=0; i<MD5str.size(); i++)
    {
      for(int j=0; j<content.size(); j++)
      {
        if(MD5str[i] == content[j].substr(0,32))
        {
          if(MD5str[i] == getstringMD5(content[j].substr(32)))
            ::write(fd, content[j].substr(32).data(), content[j].substr(32).size());
          else
          {
            std::cout << MD5str[i] << '\n';
            std::cout << content[j].size() << '\n';
            std::cout << content[j].substr(content[j].size()-4) << '\n';
          }
        }
      }
    }
  }
}

bool lsMD5InFile(std::string md5)
{
    for(int i=0; i<blockMD5Num.size(); i++)
    {
        if(blockMD5Num[i] == md5)
        {
            return true;
        }
    }
    return false;
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

void downLoadFileBlock(int conndfd, std::string argv)
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
  while(true)
  {
    if(blockMD5Num.size() == MD5str.size())
    {
      std::cout << "success" << '\n';
      return;
    }
    Buff.readFd(conndfd);
    

    while(Buff.readableBytes() >= 4)
    {
      const void *tmp = Buff.peek();
      int32_t be32 = *static_cast<const int32_t*>(tmp);
      size_t BlockFileSize = ::ntohl(be32);
      
      if(Buff.readableBytes() >= 4+36+BlockFileSize)
      {
        //std::cout << BlockFileSize << '\n';
        Buff.retrieve(4);
        std::string FileContent = Buff.readAsBlock(36+BlockFileSize);
        if(getstringMD5(FileContent.substr(36)) == FileContent.substr(0,32))
        {
          if(lsMD5InFile(FileContent.substr(0,32)))
          {
            continue;
          }
          else
          {
            blockMD5Num.push_back(FileContent.substr(0,32));
            //std::cout << blockMD5Num.size() << ':' << MD5str.size() << '\n';
            std::string file = "./" + argv + '/' + FileContent.substr(0,36);
            int fd = open(file.data(), O_RDWR|O_CREAT);
            ::write(fd, FileContent.substr(36).data(), FileContent.substr(36).size());
          }
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