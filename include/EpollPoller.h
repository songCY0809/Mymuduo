#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "EpollPoller.h"

class Channel;

class EpollPoller :public Poller
{
public:
	EpollPoller(EventLoop* loop);
	~EpollPoller() override;

	//重写基类poller的纯虚函数
	Timestamp poll(int timeoutMs, ChannelList* activeChannels)override;
	void updateChannel(Channel* channel)override;
	void removeChannel(Channel* channel)override;

private:
	//epollfd_存储epoll_create创建返回的fd
	int epollfd_;
	//存放epoll_wait返回的所有发生事件的文件描述符事件集
	//C++中可省略struct，直接使用epoll_event
	using EventList = std::vector<epoll_event>;
	EventList events_;
	//初始事件数组大小
	static const int kInitventListSize = 16;

	//填写活跃的连接
	void fillActiveChannels(int numEvents, ChannelList* activeChannels)const;
	//调用epoll_ctl更新channel通道
	void update(int operation, Channel* channel);

};