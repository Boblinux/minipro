#include "Acceptor.h"
#include "Loop.h"
#include "InetAddress.h"

#include <boost/bind.hpp>

using namespace Reactor;

Acceptor::Acceptor(Loop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(::socket(AF_INET,
                  SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                  IPPROTO_TCP)),
    acceptEvent_(loop, acceptSocket_.fd()),
    listenning_(false)
{
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.bindAddress(listenAddr);
  acceptEvent_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen()
{
  listenning_ = true;
  acceptSocket_.listen();
  acceptEvent_.enableReading();
}

void Acceptor::handleRead()
{
  InetAddress peerAddr(0);
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      ::close(connfd);
    }
  }
}

