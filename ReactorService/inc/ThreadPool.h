#ifndef REACTOR_NET_EVENTLOOPTHREADPOOL_H
#define REACTOR_NET_EVENTLOOPTHREADPOOL_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace Reactor
{

class Loop;
class LoopThread;

class ThreadPool : boost::noncopyable
{
 public:
  ThreadPool(Loop* baseLoop);
  ~ThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start();
  Loop* getNextLoop();

 private:
  Loop*                          baseLoop_;
  int                                 numThreads_;
  int                                 next_; 
  boost::ptr_vector<LoopThread>  threads_;
  std::vector<Loop*>             loops_;
};

}

#endif 
