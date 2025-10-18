#pragma once

#include <vector>
#include <unordered_map>

#include "noncopyable.h"
#include "Timestamp.h"

// 前向声明
class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心I/O复用模块
class Poller
{
public:
	using ChannelList = std::vector<Channel *>;
	Poller(EventLoop* loop);
	virtual ~Poller() = default;

	// 轮询I/O事件，必须在EventLoop线程中调用
	virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
	virtual void updateChannel(Channel* channel) = 0;
	virtual void removeChannel(Channel* cahnnel) = 0;

	// 判断channel是否在Poller中
	bool hasChannel(Channel* channel) const;

	// EventLoop可以通过该接口获取默认的I/O复用实现
	static Poller* newDefaultPoller(EventLoop* loop);

protected:
	// 存储文件描述符到Channel的映射
	using ChannelMap = std::unordered_map<int, Channel*>;
	ChannelMap channels_;

private:
	// Poller所属的EventLoop
	EventLoop* ownerLoop_;

};