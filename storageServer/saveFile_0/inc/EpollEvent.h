#ifndef REACTOR_NET_CHANNEL_H
#define REACTOR_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <poll.h>

namespace Reactor
{

class Loop;

class EpollEvent : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;
  typedef boost::function<void()> ReadEventCallback;

  EpollEvent(Loop* loop, int fd);
  ~EpollEvent();

  void handleEvent();
  void setReadCallback(const ReadEventCallback& cb) { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  bool isWriting() const { return events_ & kWriteEvent; }

  int getStatu() { return Statu_; }
  void set_Statu(int idx) { Statu_ = idx; }

  Loop* ownerLoop() { return loop_; }

 private:
  void update();

  const int kNoneEvent = 0;
  const int kReadEvent = POLLIN | POLLPRI;
  const int kWriteEvent = POLLOUT;

  Loop*          loop_;
  const int           fd_;
  int                 events_;   //care event
  int                 revents_;  //envent is now
  int                 Statu_;    // used by EPoller.

  ReadEventCallback   readCallback_;
  EventCallback       writeCallback_;
  EventCallback       errorCallback_;
  EventCallback       closeCallback_;
};

}
#endif
