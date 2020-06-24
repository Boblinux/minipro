#ifndef REACTOR_NET_INETADDRESS_H
#define REACTOR_NET_INETADDRESS_H

#include "copyable.h"

#include <string>
#include <netinet/in.h>

namespace Reactor
{

/// Wrapper of sockaddr_in. 
class InetAddress : public Reactor::copyable
{
 public:
  explicit InetAddress(uint16_t port);
  InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
  { }

  std::string toHostPort() const;
  const struct sockaddr_in& getSockAddrInet() const { return addr_; }
  void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

 private:
  struct sockaddr_in addr_;
};

}

#endif
