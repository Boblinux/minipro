#ifndef REACTOR_NET_EPOLLER_H
#define REACTOR_NET_EPOLLER_H

#include <map>
#include <vector>
#include "Loop.h"

struct epoll_event;

namespace Reactor
{

class EpollEvent;

class EPoller : boost::noncopyable
{
 public:
  typedef std::vector<EpollEvent*> EventlList;

  EPoller(Loop* loop);
  ~EPoller();

  void epoll(int timeoutMs, EventlList* activeEvents);
  void updateEvent(EpollEvent* event);
  void removeEvent(EpollEvent* event);

 private:
  static const int kInitEventListSize = 16;

  void pushActiveEvents(int numEvents,
                          EventlList* activeEvents) const;
  void update(int operation, EpollEvent* event);

  typedef std::vector<struct epoll_event> EventList;
  typedef std::map<int, EpollEvent*> DistributerMap;

  Loop*             ownerLoop_;
  int               epollfd_;
  EventList         events_;
  DistributerMap    Distributers_;
};

}
#endif 
