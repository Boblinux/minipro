#include "Socket.h"

#include "InetAddress.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero
#include <sys/socket.h>

using namespace Reactor;

Socket::~Socket() { ::close(sockfd_); }

void Socket::bindAddress(const InetAddress& addr)  
{ ::bind(sockfd_, (sockaddr*)(&addr.getSockAddrInet()), sizeof addr.getSockAddrInet()); }
void Socket::listen() { ::listen(sockfd_, SOMAXCONN);}
int Socket::accept(InetAddress* peeraddr) 
{
  struct sockaddr_in addr;
  bzero(&addr, sizeof addr);
  socklen_t addrlen = sizeof addr;
  int connfd = ::accept4(sockfd_, (sockaddr*)(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0)
  {
    peeraddr->setSockAddrInet(addr);
  }
  return connfd;
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof optval);
}

void Socket::shutdownWrite()
{
  ::shutdown(sockfd_, SHUT_WR);
}

void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
}