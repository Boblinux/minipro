#ifndef REACTOR_NET_LOOP_H
#define REACTOR_NET_LOOP_H

#include "Callbacks.h"
#include <sys/syscall.h>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>
#include <mutex>
#include <thread>
#include <pthread.h>

namespace Reactor
{

class EpollEvent;
class EPoller;

class Loop : boost::noncopyable
{
 public:
  typedef boost::function<void()> Functor;

  Loop();
  ~Loop();

  /// Loops forever. 
  void loop();
  void quit();

  /// It wakes up the loop, and run the cb.
  void runInLoop(const Functor& cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  void queueInLoop(const Functor& cb);

  // internal use only
  void wakeup();
  void updateEvent(EpollEvent* epollEvent);
  void removeEvent(EpollEvent* epollEvent);

  bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }

 private:

  void handleRead();  // waked up
  void doFunctors();

  typedef std::vector<EpollEvent*>   EventlList;

  bool                            looping_;
  bool                            quit_;
  bool                            callFunctors_; 
  const std::thread::id           threadId_;
  int                             wakeupFd_;

  // we don't expose EpollEvent/EPoller to client.
  boost::scoped_ptr<EPoller>      epoller_;
  boost::scoped_ptr<EpollEvent>   wakeupEvent_;
  EventlList                      activeEvents_;
  std::mutex                      mutex_;
  std::vector<Functor>            Functors_; 
};

}
#endif 
