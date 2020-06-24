#include "Loop.h"
#include "EpollEvent.h"
#include "EPoller.h"

#include <boost/bind.hpp>
#include <iostream>
#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>

using namespace Reactor;

Loop* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

static int createEventfd()
{
  return ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
}

Loop::Loop()
  : looping_(false),
    quit_(false),
    callFunctors_(false),
    threadId_(std::this_thread::get_id()),
    wakeupFd_(createEventfd()),
    epoller_(new EPoller(this)),
    wakeupEvent_(new EpollEvent(this, wakeupFd_))
{
  printf("thread id: %lu created success\n", threadId_);
  t_loopInThisThread = this;
  wakeupEvent_->setReadCallback(boost::bind(&Loop::handleRead, this));
  wakeupEvent_->enableReading();
}

Loop::~Loop()
{
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

void Loop::loop()
{
  looping_ = true;
  quit_ = false;

  while (!quit_)
  {
    activeEvents_.clear();
    epoller_->epoll(kPollTimeMs, &activeEvents_);
    for (EventlList::iterator it = activeEvents_.begin();
        it != activeEvents_.end(); ++it)
    {
      (*it)->handleEvent();
    }
    doFunctors();
  }
  looping_ = false;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           

void Loop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void Loop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void Loop::queueInLoop(const Functor& cb)
{
  {
    std::lock_guard<std::mutex> guard(mutex_);
    Functors_.push_back(cb);
  }

  if (!isInLoopThread() || callFunctors_)
  {
    wakeup();
  }
}

void Loop::updateEvent(EpollEvent* epollEvent)
{
  epoller_->updateEvent(epollEvent);
}

void Loop::removeEvent(EpollEvent* epollEvent)
{
  epoller_->removeEvent(epollEvent);
}

void Loop::wakeup()
{
  uint64_t one = 1;
  ::write(wakeupFd_, &one, sizeof one);
}

void Loop::handleRead()
{
  uint64_t one = 1;
  ::read(wakeupFd_, &one, sizeof one);
}

void Loop::doFunctors()
{
  std::vector<Functor> functors;
  callFunctors_ = true;

  {
    std::lock_guard<std::mutex> guard(mutex_);
    functors.swap(Functors_);
  }

  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callFunctors_ = false;
}

