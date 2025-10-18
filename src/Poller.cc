#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop) : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel* channel) const
{
    auto it = channels_.find(channel->fd());
    //双重验证，防止一个文件描述符对应多个channel的情况
    //确保文件描述符fd存在并且就是特定的channel对象
    return it != channels_.end() && it->second == channel;
}