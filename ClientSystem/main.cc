#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>  
#include <dirent.h>
#include <iostream>
#include <openssl/md5.h>
#include <fcntl.h>
#include <sys/wait.h>   
#include <sys/epoll.h>  
#include "File.h"
#include "upLoad.h"
#include "downLoad.h"
#include "mysocket.h"

#define PORT                    9981 
#define BUFFERSIZE              1024*1024 

using namespace Md;

void printfMD5vec(std::vector<std::string> MD5str)
{
    for(int i=0; i<MD5str.size(); i++)
    {
        std::cout << MD5str[i] << '\n';
    }
}

int main(int argc, char *argv[]) {
    if(argc != 3) { 
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
    int conndfd;
    int conn[3];
    std::string fileName = argv[1];
    std::string opr = argv[2];
    //std::cout << opr;
    if(opr == "upload")
    {
        conndfd = mySocket1(9981);
        ::send(conndfd, "upppload", 8, MSG_NOSIGNAL);
        upLoadFile(conndfd, fileName);
    }
    else if(opr == "download")
    {
        if(access(argv[1], 0) == 0)
        {
            DIR *dir;
            struct dirent *ptr;
            if ((dir=opendir(argv[1])) == NULL)
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
                    blockMD5Num.push_back(Name.substr(0,32));
                }
            }
            closedir(dir);
        }
        else
        {
            mkdir(argv[1], S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        }
        

        conndfd = mySocket2(9981);
        ::send(conndfd, "download", 8, MSG_NOSIGNAL);
        downLoadMD5(conndfd, MD5str, argv[1]);

        if(blockMD5Num.size() == MD5str.size())
        {
            combinateFile(MD5str, argv[1], argv[1]);
            exit(0);
        }
        
        conn[0] = mySocket2(9982);
        conn[1] = mySocket2(9983);
        conn[2] = mySocket2(9984);
        ::send(conn[0], "download", 8, MSG_NOSIGNAL);
        ::send(conn[1], "download", 8, MSG_NOSIGNAL);
        ::send(conn[2], "download", 8, MSG_NOSIGNAL);
        std::thread thr1(std::bind(downLoadFileBlock, conn[0], argv[1]));
        std::thread thr2(std::bind(downLoadFileBlock, conn[1], argv[1]));
        std::thread thr3(std::bind(downLoadFileBlock, conn[2], argv[1]));
        thr1.detach();
        thr2.detach();
        thr3.detach();
        
        
        while(blockMD5Num.size() != MD5str.size())
        {
            std::cout << blockMD5Num.size() << ":" <<MD5str.size() << '\n';
        }
        combinateFile(MD5str, argv[1], argv[1]);
        close(conn[0]); 
        close(conn[1]); 
        close(conn[2]); 
    }
    else
    {
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
  
    close(conndfd); 
    return 0;
}