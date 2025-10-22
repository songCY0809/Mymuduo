#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>

#include "EventLoop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

//防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;

//定义默认的Poller I/O复用接口的超时时间
const int kPollTimeMs = 10000;

int createEventfd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG_FATAL("eventfd error:%d\n", errno);
	}
	return evtfd;
}

EventLoop::EventLoop()
	: looping_(false)
	, quit_(false)
	, callingPendingFunctors_(false)
	, threadId_(CurrentThread::tid())
	, poller_(Poller::newDefaultPoller(this))
	, wakeupFd_(createEventfd())
	, wakeupChannel_(new Channel(this, wakeupFd_))
{
	LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
	if (t_loopInThisThread)
	{
		LOG_FATAL("Another EvnetLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
	}
	else
	{
		t_loopInThisThread = this;
	}

	wakeupChannel_->setReadCallback(
		std::bind(&EventLoop::handleRead, this));	//设置wakeupfd的事件类型和发生事件后的回调
	wakeupChannel_->enableReading();				//每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了
}

EventLoop::~EventLoop()
{
	wakeupChannel_->disableAll();	//给Channel移除所有感兴趣的事件
	wakeupChannel_->remove();		//把Channel从EventLoop上删除掉
	::close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
	looping_ = true;
	quit_ = false;

	LOG_INFO("EventLoop %p start looping\n", this);

	while (!quit_)
	{
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
		for (Channel* channel : activeChannels_)
		{
			//Poller监听哪些channel发生了事件，然后上报给EventLoop，通知channel处理相应事件
			channel->handleEvent(pollReturnTime_);
		}
		doPendingFunctors();
	}
	LOG_INFO("EventLoop %p stop looping.\n", this);
	looping_ = false;
}

void EventLoop::quit()
{
	quit_ = true;

	if (!isInLoopThread())
	{
		wakeup();
	}
}

void EventLoop::runInLoop(Functor cb)
{
	//在当前EventLoop中执行回调
	if (isInLoopThread())
	{
		cb();
	}
	else//在非当前EventLoop中执行cb，就需要唤醒EventLoop所在线程执行cb
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(Functor cb)
{
	{
		std::unique_lock<std::mutex> lock(mutex_);
		pendingFunctors_.emplace_back(cb);
	}

	if (!isInLoopThread() || callingPendingFunctors_)
	{
		wakeup();
	}
}

void EventLoop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
	{
		LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8\n", n);
	}
}

void EventLoop::wakeup()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one))
	{
		LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8\n", n);
	}
}

void EventLoop::updateChannel(Channel* channel)
{
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
	poller_->removeChannel(channel);
}


bool EventLoop::hasChannel(Channel* channel)
{
	return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
		std::unique_lock<std::mutex> lock(mutex_);
		functors.swap(pendingFunctors_);//交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 
										//如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
	}
	
	for (const Functor& functor : functors)
	{
		functor();//执行当前loop需要执行的回调函数
	}

	callingPendingFunctors_ = false;
}
