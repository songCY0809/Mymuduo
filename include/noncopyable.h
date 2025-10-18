#pragma once

//noncopyable及其派生类禁止拷贝构造与赋值构造
class noncopyable
{
public:
	noncopyable(const noncopyable&) = delete;
	//muduo赋值运算符重载返回值为void
	// void operator=(const noncopyable &) = delete;
	noncopyable& operator=(const noncopyable&) = delete;

protected:
	noncopyable() = default;
	~noncopyable() = default;
};
