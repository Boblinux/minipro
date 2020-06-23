#ifndef REACTOR_NET_SOCKET_H
#define REACTOR_NET_SOCKET_H

#include <boost/noncopyable.hpp>
#include <unistd.h>

namespace Reactor
{

class InetAddress;

/// Wrapper of socket file descriptor. It closes the sockfd when desctructs.
class Socket : boost::noncopyable
{
 public:
  explicit Socket(int sockfd)
    : sockfd_(sockfd)
  { 
    setTcpNoDelay(true);
    setKeepAlive(true);
  }

  ~Socket();

  int fd() const { return sockfd_; }

  void bindAddress(const InetAddress& localaddr);
  void listen();
  int accept(InetAddress* peeraddr);

  /// Enable/disable SO_REUSEADDR
  void setReuseAddr(bool on);
  void shutdownWrite();

  /// enable/disable Nagle's algorithm.
  void setTcpNoDelay(bool on);
  void setKeepAlive(bool on);

 private:
  const int sockfd_;
};

}
#endif 
