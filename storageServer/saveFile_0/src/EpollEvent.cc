#include "EpollEvent.h"
#include "Loop.h"

#include <sstream>
#include <iostream>

using namespace Reactor;

EpollEvent::EpollEvent(Loop* loop, int fdArg)
  : loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    Statu_(-1)
{
}

EpollEvent::~EpollEvent()
{
}

void EpollEvent::update()
{
  loop_->updateEvent(this);
}

void EpollEvent::handleEvent()
{
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (closeCallback_) closeCallback_();
  }
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorCallback_) errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readCallback_) readCallback_();
  }
  if (revents_ & POLLOUT) {
    if (writeCallback_) writeCallback_();
  }
}
