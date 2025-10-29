#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0;//空事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;//读事件
const int Channel::kWriteEvent = EPOLLOUT;//写事件

Channel::Channel(EventLoop* loop, int fd)
	: loop_(loop)
	, fd_(fd)
	, events_(0)
	, revents_(0)
	, index_(-1)
	, tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
	//tie_是weak_ptr，weak_ptr赋值为shared_ptr，防止循环引用。
	tie_ = obj;
	tied_ = true;
}

//update 和remove => EpollPoller 更新channel在poller中的状态
void Channel::update()
{
	//通过channel所属的eventloop，调用poller的相应方法，注册fd中的events事件
	loop_->updateChannel(this);
}

void Channel::remove()
{
	loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	if (tied_)
	{
		std::shared_ptr<void> guard = tie_.lock();
		if (guard)
		{
			handleEventWithGuard(receiveTime);
		}
		//如果weak_ptr的lock()失败，说明tcp连接对象已经不存在了，不做任何操作
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	LOG_INFO("channel handleEvent revents:%d\n", revents_);

	//关闭
	if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
	{
		if (closeCallback_)
		{
			closeCallback_();
		}
	}
	//错误
	if (revents_ & EPOLLERR)
	{
		if (errorCallback_)
		{
			errorCallback_();
		}
	}
	//读
	if (revents_ & (EPOLLIN | EPOLLPRI))
	{
		if (readCallback_)
		{
			readCallback_(receiveTime);
		}
	}
	//写
	if (revents_ & EPOLLOUT)
	{
		if (writeCallback_)
		{
			writeCallback_();
		}
	}
}


