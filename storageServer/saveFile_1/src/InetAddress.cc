#include "InetAddress.h"

#include <strings.h>  // bzero
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/static_assert.hpp>

using namespace Reactor;

static const in_addr_t kInaddrAny = INADDR_ANY;

InetAddress::InetAddress(uint16_t port)
{
  bzero(&addr_, sizeof addr_);
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = htonl(kInaddrAny);
  addr_.sin_port = htons(port);
}

std::string InetAddress::toHostPort() const
{
  char buf[32];
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr_.sin_addr, host, sizeof host);
  uint16_t port = ntohs(addr_.sin_port);
  snprintf(buf, sizeof buf, "%s:%u", host, port);

  return buf;
}

