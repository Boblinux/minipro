#include "ThreadPool.h"

#include "Loop.h"
#include "LoopThread.h"
#include <boost/bind.hpp>

using namespace Reactor;

ThreadPool::ThreadPool(Loop* baseLoop)
  : baseLoop_(baseLoop),
    numThreads_(0),
    next_(0)
{
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::start()
{
  for (int i = 0; i < numThreads_; ++i)
  {
    LoopThread* t = new LoopThread;
    threads_.push_back(t);
    loops_.push_back(t->startLoop());
  }
}

Loop* ThreadPool::getNextLoop()
{
  Loop* loop = baseLoop_;

  if (!loops_.empty())
  {
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

