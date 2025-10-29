#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

const int kNew = -1;	//某个channel还未添加至Poller（channel成员index_初始化为-1）
const int kAdded = 1;	//某个channel已经添加至Poller
const int kDeleted = 2; //某个channel已经从Poller删除

EpollPoller::EpollPoller(EventLoop* loop)
	:Poller(loop)
	,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
	,events_(kInitventListSize)//vector<epoll_event>(16)
{
	if (epollfd_ < 0)
	{
		LOG_FATAL("epoll_create error:%d \n", errno);
	}
}

EpollPoller::~EpollPoller()
{
	::close(epollfd_);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
	LOG_DEBUG("func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

	int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
	int saveErrno = errno;
	Timestamp now(Timestamp::now());

	if (numEvents > 0)
	{
		LOG_DEBUG("%d events happened\n", numEvents);
		fillActiveChannels(numEvents, activeChannels);
		//扩容操作
		if (numEvents == events_.size())
		{
			events_.resize(events_.size() * 2);
		}
	}
	else if (numEvents == 0)
	{
		LOG_DEBUG("%s timeout!\n", __FUNCTION__);
	}
	else
	{
		if (saveErrno != EINTR)
		{
			errno = saveErrno;
			LOG_ERROR("EpollPoller:poll() error!");
		}
	}
	return now;
}

//channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
void EpollPoller::updateChannel(Channel* channel)
{
	const int index = channel->index();
	LOG_INFO("func%s =>fd=%d events=%d index=%d\n", __FUNCTION__, channel->fd(), channel->events(), index);

	if (index == kNew || index == kDeleted)
	{
		if (index == kNew)
		{
			int fd = channel->fd();
			channels_[fd] = channel;
		}
		else//index == kDeleted
		{
		}
		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD, channel);
	}
	else//channel已经在Poller中注册过了
	{
		int fd = channel->fd();
		if (channel->isNoneEvent())
		{
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		}
		else
		{
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

void EpollPoller::removeChannel(Channel* channel)
{
	int fd = channel->fd();
	channels_.erase(fd);

	LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

	int index = channel->index();
	if (index == kAdded)
	{
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
	for (int i = 0; i < numEvents; ++i)
	{
		Channel* channel = static_cast<Channel *>(events_[i].data.ptr);
		channel->set_revents(events_[i].events);
		//EventLoop拿到了它的Poller给它返回的所有发生事件的列表
		activeChannels->push_back(channel);
	}
}

void EpollPoller::update(int operation, Channel* channel)
{
	epoll_event event;
	::memset(&event, 0, sizeof(event));

	int fd = channel->fd();

	event.events = channel->events();
	event.data.fd = fd;
	event.data.ptr = channel;

	if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
	{
		if (operation == EPOLL_CTL_DEL)
		{
			LOG_ERROR("epoll_ctl del error:%d\n", errno);
		}
		else
		{
			LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
		}
	}
}