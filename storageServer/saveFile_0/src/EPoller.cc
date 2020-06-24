#include "EPoller.h"
#include "EpollEvent.h"

#include <boost/static_assert.hpp>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <iostream>

using namespace Reactor;

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPoller::EPoller(Loop* loop)
  : ownerLoop_(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
}

EPoller::~EPoller()
{
  ::close(epollfd_);
}

void EPoller::epoll(int timeoutMs, EventlList* activeEvents)
{
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  pushActiveEvents(numEvents, activeEvents);
  if (static_cast<size_t>(numEvents) == events_.size())
  {
    events_.resize(events_.size()*2);
  }
}

void EPoller::pushActiveEvents(int numEvents, EventlList* activeEvents) const
{
  for (int i = 0; i < numEvents; ++i)
  {
    EpollEvent* epollEvent = static_cast<EpollEvent*>(events_[i].data.ptr);
    epollEvent->set_revents(events_[i].events);
    activeEvents->push_back(epollEvent);
  }
}

void EPoller::updateEvent(EpollEvent* epollEvent)
{
  const int getStatu = epollEvent->getStatu();
  if (getStatu == kNew || getStatu == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = epollEvent->fd();
    if (getStatu == kNew)
    {
      Distributers_[fd] = epollEvent;
    }
    epollEvent->set_Statu(kAdded);
    update(EPOLL_CTL_ADD, epollEvent);
  }
  else
  {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = epollEvent->fd();
    (void)fd;
    if (epollEvent->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, epollEvent);
      epollEvent->set_Statu(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, epollEvent);
    }
  }
}

void EPoller::removeEvent(EpollEvent* epollEvent)
{
  int fd = epollEvent->fd();
  int getStatu = epollEvent->getStatu();
  size_t n = Distributers_.erase(fd);
  (void)n;
  if (getStatu == kAdded)
  {
    update(EPOLL_CTL_DEL, epollEvent);
  }
  epollEvent->set_Statu(kNew);
}

void EPoller::update(int operation, EpollEvent* epollEvent)
{
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = epollEvent->events();
  event.data.ptr = epollEvent;
  int fd = epollEvent->fd();
  ::epoll_ctl(epollfd_, operation, fd, &event);
}

