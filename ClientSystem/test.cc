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
#include "File.h"
#include "BlockFile.h"

#define PORT                    9981 
#define BUFFERSIZE              1024*1024 

using namespace Md;

void setTcpNoDelay(int sockfd_, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
}

void setKeepAlive(int sockfd_, bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
}

inline int mySocket()
{
    int conndfd;
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serverAddr.sin_port=htons(PORT);
    conndfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    setTcpNoDelay(conndfd, true);
    setKeepAlive(conndfd, true);
    connect(conndfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    return conndfd;
}

// Get the size of the file by its file descriptor
unsigned long getFileSize(int fd) {
    struct stat statbuf;
    if(fstat(fd, &statbuf) < 0) exit(-1);
    return statbuf.st_size;
}

int main(int argc, char *argv[]) {
    if(argc != 2) { 
        printf("Please input: ./test filename\n");
        exit(-1);
    }
    static char buffer[BUFFERSIZE];
    bzero(buffer, sizeof(buffer)); 
    int n;
    int conndfd = mySocket();
    int fd = open(argv[1], O_RDONLY);
    unsigned long file_size = getFileSize(fd);
    Md::File File(fd, conndfd, file_size); 
    int blockNum = File.getFileBlockNum();
    std::cout << "File num: " << blockNum << " File MD5: "<< File.getFileMD5()  << std::endl;

    File.sendFileinfo(argv[1]); //1、seng Fileinfo

    if((n = recv(conndfd, buffer, BUFFERSIZE, MSG_NOSIGNAL)) > 0)
    {
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer)); 
        std::cout << idstr << std::endl;
        if(idstr == "success")
        {
            close(conndfd); 
            return 0;  //5、success
        }
        
        std::string::size_type position = idstr.find(':');
        if(position != idstr.npos)
        {
            File.updateuploadId(idstr); //2、if file existence, updateuploadId
        }
        else
        {
            File.sendFileBlockMD5info(); //3、else sendFileBlockMD5info 
        }
    }

    File.sendFileBlock();   //4、sendFileBlock

    while((n = recv(conndfd, buffer, BUFFERSIZE, MSG_NOSIGNAL)) > 0)
    {
        std::cout << buffer << std::endl;
        std::string idstr = buffer;
        bzero(buffer, sizeof(buffer));
        if(idstr == "success")
        {
            break;  //5、success
        }
        else
        {
            File.updateuploadId(idstr); // goto 2、updateuploadId
            File.sendFileBlock(); 
        }
    }
    
    close(conndfd); 
    return 0;
}