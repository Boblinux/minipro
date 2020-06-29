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
        downLoad(conndfd, fileName);
    }
    else
    {
        printf("Please input: ./test filename upload/download\n");
        exit(-1);
    }
  
    system("pause");
    close(conndfd); 
    return 0;
}