#include "LoopThread.h"
#include "Loop.h"

#include <boost/bind.hpp>

using namespace Reactor;

LoopThread::LoopThread()
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&LoopThread::threadFunc, this)),
    mutex_(),
    cond_()
{
}

LoopThread::~LoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}

Loop* LoopThread::startLoop()
{

  {
    std::unique_lock<std::mutex> unique(mutex_);
    cond_.wait(unique);
  }

  return loop_;
}

void LoopThread::threadFunc()
{
  Loop loop;

  {
    std::unique_lock<std::mutex> unique(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  
  loop.loop();
}

