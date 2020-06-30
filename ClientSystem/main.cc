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
std::vector<std::string> MD5str;
std::vector<std::string> MD5;
void printfMD5vec()
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
        conndfd = mySocket2(9981);
        ::send(conndfd, "download", 8, MSG_NOSIGNAL);
        downLoadMD5(conndfd, MD5str, argv[1]);
        printfMD5vec();
        conndfd = mySocket2(9982);
        ::send(conndfd, "download", 8, MSG_NOSIGNAL);
        downLoadFileBlock(conndfd, MD5str, argv[1]);
        //downLoad(conndfd, fileName);
    }
    else
    {
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
  
    close(conndfd); 
    return 0;
}