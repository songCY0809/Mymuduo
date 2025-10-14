#pragma once

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

// 前向声明
class EventLoop;

// Channel类是一个sockfd封装器
class Channel :noncopyable
{
public:
	using EventCallback = std::function<void()>;
	using ReadEventCallback = std::function<void(Timestamp)>;

	Channel(EventLoop* loop, int fd);
	~Channel();

	// 回调函数链，在eventloop中使用
	void handleEvent(Timestamp receiveTime);

	// 回调函数链，联系eventloop与channel
	void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
	void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
	void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
	void setErrorCallback(EventCallback cb) { errorCallbakc_ = std::move(cb); }

	// tie_用于解决channel和tcpconnection生命周期问题，
	// 防止在tie_.lock()前其对象就被销毁了，导致错误调用
	void tie(const std::shared_ptr<void> &);

	//获取和设置私有成员的函数
	int fd() const { return fd_; }
	int events()const { return events_; }
	void set_revents(int revt) { revents_ = revt; }
	int index() { return index_; }
	void set_index(int idx) { index_ = idx; }

	//设置fd相应状态
	void enableReading() { events_ |= kReadEvent; update(); }
	void disableReading() { events_ |= ~kReadEvent; update(); }
	void enableWriting() { evens_ |= kWriteEvent; update(); }
	void disableWriting() { events_ |= ~kWriteEvent; update(); }
	void disableAll() { events_ |= kNoneEvent; update(); }

	//返回fd当前状态
	bool isNoneEvent()const { return events_ == kNoneEvent; }
	bool isReading()const { return events_ & kReadEvent; }
	bool isWriting()const { return events_ & kWriteEvent; }

	//one loop per thread
	EventLoop* ownerLoop() { return loop_; }
	void remove();


private:
	void update();
	void handleEventWithGuard(Timestamp receiveTime);

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;//事件循环
	const int fd_;//fd,Poller监听对象
	int events_;//注册fd感兴趣的事件
	int revents_;//Poller返回的实际发生的事件
	int index_;

	std::weak_ptr<void> tie_;
	bool tied_;

	//channel回调函数，启动回调函数链的执行
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;

};