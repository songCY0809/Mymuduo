#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool :noncopyable
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;
	EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
	~EventLoopThreadPool();

	void setThreadNum(int numThreads) { numThreads_ = numThreads; }

	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	//如果工作在多线程中，baseLoop_（mainLoop）会默认以轮询方法分配Channel给subLoop
	EventLoop* getNextLoop();

	//获取所有的EventLoop
	std::vector<EventLoop*> getAllLoops();

	//是否已启动
	bool started()const { return started_; }
	//获取名字
	const std::string name()const { return name_; }

private:
	EventLoop* baseLoop_;	//用户使用muduo创建的loop,如果线程数为1,
							//那直接使用用户创建的loop,否则创建多EventLoop
	std::string name_;		//线程池名称，通常由用户指定，线程池中EventLoopThread名称依赖于线程池名称
	bool started_;			//是否已经启动的标志
	int numThreads_;		//线程池中线程数量
	int next_;				//新连接到来，所选择的EventLoop的索引
	std::vector<std::unique_ptr<EventLoopThread>>threads_;	//I/O线程的列表
	std::vector<EventLoop*> loops_;							//线程池中EventLoop的列表，
															//指向EventLoopThread线程函数创建的EventLoop对象
};