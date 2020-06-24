#include "TcpConnection.h"
#include "EpollEvent.h"
#include "Loop.h"
#include "Socket.h"

#include <iostream>
#include <boost/bind.hpp>
#include <stdio.h>

using namespace Reactor;

TcpConnection::TcpConnection(Loop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    epollEvent_(new EpollEvent(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
  epollEvent_->setReadCallback(boost::bind(&TcpConnection::handleRead, this));
  epollEvent_->setWriteCallback(boost::bind(&TcpConnection::handleWrite, this));
  epollEvent_->setCloseCallback(boost::bind(&TcpConnection::handleClose, this));
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::send(const std::string& message)
{
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop, this, message));
    }
  }
}

void TcpConnection::sendInLoop(const std::string& message)
{
  ssize_t nwrote = 0;
  // if no thing in output queue, try writing directly
  if (!epollEvent_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(epollEvent_->fd(), message.data(), message.size());
    if (static_cast<size_t>(nwrote) < message.size()) {
      printf("I am going to write more data\n");
    }
  }

  if (static_cast<size_t>(nwrote) < message.size()) {
    outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
    if (!epollEvent_->isWriting()) {
      epollEvent_->enableWriting();
    }
  }
}

void TcpConnection::shutdownInLoop()
{
  if (!epollEvent_->isWriting())
  {
    socket_->shutdownWrite();
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
  setState(kConnected);
  epollEvent_->enableReading();
  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  setState(kDisconnected);
  epollEvent_->disableAll();
  connectionCallback_(shared_from_this());
  loop_->removeEvent(get_pointer(epollEvent_));
}

void TcpConnection::handleRead()
{
  ssize_t n = inputBuffer_.readFd(epollEvent_->fd());
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_);
  } else if (n == 0) {
    handleClose();
  } 
}

void TcpConnection::handleWrite()
{
  if (epollEvent_->isWriting()) {
    ssize_t n = ::write(epollEvent_->fd(),
                        outputBuffer_.peek(),
                        outputBuffer_.readableBytes());
    outputBuffer_.retrieve(n);
    if (outputBuffer_.readableBytes() == 0) {
      epollEvent_->disableWriting();
      if (state_ == kDisconnecting) {
        shutdownInLoop();
      }
    }
  }
}

void TcpConnection::handleClose()
{
  epollEvent_->disableAll();
  closeCallback_(shared_from_this());
}
