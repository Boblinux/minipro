#include "TcpServer.h"
#include "Acceptor.h"
#include "Loop.h"
#include "ThreadPool.h"

#include <boost/bind.hpp>
#include <iostream>
#include <stdio.h>  // snprintf

using namespace Reactor;

TcpServer::TcpServer(Loop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    name_(listenAddr.toHostPort()),
    acceptor_(new Acceptor(loop, listenAddr)),
    threadPool_(new ThreadPool(loop)),
    started_(false),
    nextConnId_(1)
{
  acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThreads)
{
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
    threadPool_->start();
  }

  if (!acceptor_->listenning())
  {
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;
  
  //printf("New connection [%s] from %s\n",connName.data(), peerAddr.toHostPort().data());

  struct sockaddr_in localAddr;
  bzero(&localAddr, sizeof localAddr);
  socklen_t addrlen = sizeof(localAddr);
  ::getsockname(sockfd, (sockaddr*)(&localAddr), &addrlen);

  // get epoll with zero timeout to double confirm the new connection
  Loop* ioLoop = threadPool_->getNextLoop();
  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
  ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  //printf("TcpServer::removeConnectionInLoop [%s] - connection %s\n",name_.data(), conn->name().data());
  connections_.erase(conn->name());
  Loop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
}

