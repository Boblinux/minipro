#ifndef REACTOR_NET_TCPSERVER_H
#define REACTOR_NET_TCPSERVER_H

#include "Callbacks.h"
#include "TcpConnection.h"

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace Reactor
{

class Acceptor;
class Loop;
class ThreadPool;

class TcpServer : boost::noncopyable
{
 public:

  TcpServer(Loop* loop, const InetAddress& listenAddr);
  ~TcpServer();  // force out-line dtor, for scoped_ptr members.

  /// Set the number of threads.
  /// - 0 means all I/O in loop's thread, no thread will created.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, 
  void setThreadNum(int numThreads);

  void start();

  void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
  void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

 private:
  void newConnection(int sockfd, const InetAddress& peerAddr);
  void removeConnection(const TcpConnectionPtr& conn);
  void removeConnectionInLoop(const TcpConnectionPtr& conn);
  typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

  Loop*                                loop_;      // the acceptor loop
  const std::string                         name_;
  boost::scoped_ptr<Acceptor>               acceptor_;  // avoid revealing Acceptor
  boost::scoped_ptr<ThreadPool>             threadPool_;
  ConnectionCallback                        connectionCallback_;
  MessageCallback                           messageCallback_;
  bool                                      started_;
  int                                       nextConnId_;  // always in loop thread
  ConnectionMap                             connections_;
};

}

#endif
