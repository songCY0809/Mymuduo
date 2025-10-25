#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
	: loop_(nullptr)
	, exiting_(false)
	, thread_(std::bind(&EventLoopThread::threadFunc, this), name)
	, mutex_()
	, cond_()
	, callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
	exiting_ = true;
	if (loop_ != nullptr)
	{
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop()
{
	thread_.start();

	EventLoop* loop = nullptr;
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this]() {return loop_ != nullptr; });
		loop = loop_;
	}
	return loop;
}

void EventLoopThread::threadFunc()
{
	EventLoop loop;	//创建一个独立的EventLoop对象,和上面的线程是一一对应的,即one loop per thread

	if (callback_)
	{
		callback_(&loop);
	}

	{
		std::unique_lock<std::mutex> lock(mutex_);
		loop_ = &loop;
		cond_.notify_one();
	}
	loop.loop();	//执行Eventloop的loop（），开启底层的Poller的poll（）
	std::unique_lock<std::mutex> lock(mutex_);
	loop_ = nullptr;
}
