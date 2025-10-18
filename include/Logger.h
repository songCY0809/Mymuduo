#pragma once

#include <string>

#include "noncopyable.h"

#define LOG_INFO(logmsgFormat, ...)							\
	do														\
	{														\
		Logger& logger = Logger::instance();				\
		logger.setLogLevel(INFO);							\
		char buf[1024]={0};									\
		snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);	\
		logger.log(buf);									\
	}while (0)												\

#define LOG_ERROR(logmsgFormat, ...)						\
	do														\
	{														\
		Logger& logger = Logger::instance();				\
		logger.setLogLevel(ERROR);							\
		char buf[1024]={0};									\
		snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);	\
		logger.log(buf);									\
	}while (0)												\

#define LOG_FATAL(logmsgFormat, ...)						\
	do														\
	{														\
		Logger& logger = Logger::instance();				\
		logger.setLogLevel(FATAL);							\
		char buf[1024]={0};									\
		snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);	\
		logger.log(buf);									\
		exit(-1);											\
	}while (0)												\

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                      \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLogLevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif

//日志等级
enum LogLevel
{
	INFO, //普通信息
	ERROR,//错误信息
	FATAL,//core dump信息
	DEBUG //调试信息
};

//日志类
class Logger :noncopyable
{
public:
	//获取日志的唯一实例对象，单例
	static Logger& instance();
	//设置日志级别
	void setLogLevel(int level);
	//写日志
	void log(std::string msg);

private:
	int logLevel_;
};