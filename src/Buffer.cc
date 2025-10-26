#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
	//栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，
	//待buffer_重新分配足够空间后，在把数据交换给buffer_
	char extrabuf[65536] = { 0 };

	//使用iovec分配两个连续的缓冲区
	struct iovec vec[2];
	const size_t writable = writableBytes();//这是Buffer底层缓冲区剩余的可写空间大小,
											//不一定能完全存储从fd读出的数据

	//第一块缓冲区，指向可写空间
	vec[0].iov_base = begin() + writerIndex_;
	vec[0].iov_len = writable;
	//第二块缓冲区，指向栈空间
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof(extrabuf);

	const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
	const ssize_t n = ::readv(fd, vec, iovcnt);

	if (n < 0)
	{
		*saveErrno = errno;
	}
	else if (n <= writable)//Buffer的可写缓冲区已经够存储读出来的数据了
	{
		writerIndex_ += n;
	}
	else
	{
		writerIndex_ = buffer_.size();
		append(extrabuf, n - writable);//对buffer_扩容,并将extrabuf存储的另一部分数据追加至buffer_
	}

	return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
	ssize_t n = ::write(fd, peek(), readableBytes());
	if (n < 0)
	{
		*saveErrno = errno;
	}

	return n;
}
