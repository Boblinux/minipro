#ifndef REACTOR_NET_ACCEPTOR_H
#define REACTOR_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "EpollEvent.h"
#include "Socket.h"

namespace Reactor
{

class Loop;
class InetAddress;

/// Acceptor of incoming TCP connections.
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(Loop* loop, const InetAddress& listenAddr);

  void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  Loop*              loop_;
  Socket                  acceptSocket_;
  EpollEvent              acceptEvent_;
  NewConnectionCallback   newConnectionCallback_;
  bool                    listenning_;
};

}

#endif
