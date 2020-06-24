#ifndef REACTOR_NET_EVENTLOOPTHREAD_H
#define REACTOR_NET_EVENTLOOPTHREAD_H

#include <boost/noncopyable.hpp>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace Reactor
{

class Loop;

class LoopThread : boost::noncopyable
{
 public:
  LoopThread();
  ~LoopThread();
  Loop* startLoop();

 private:
  void threadFunc();

  Loop*                loop_;
  bool                      exiting_;
  std::thread               thread_;
  std::mutex                mutex_;
  std::condition_variable   cond_;
};

}

#endif 

