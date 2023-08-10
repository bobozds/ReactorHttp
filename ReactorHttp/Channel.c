#include "Channel.h"
#include <stdlib.h>

struct Channel* initChannel(int fd, int events, handleFunc read, handleFunc write, handleFunc destroy, void* arg)
{
	struct Channel* c = (struct Channel*)malloc(sizeof(struct Channel));
	c->fd = fd;
	c->events = events;
	c->readCallback = read;
	c->writeCallback = write;
	c->destroyCallback = destroy;
	c->arg = arg;
	return c;
}

void writeEventEnable(struct Channel* c, bool flag)
{
	if (flag)
	{
		c->events |= WriteEvent;  //��λ�򣬰�λ����1��λ�ñ��1  
	}
	else {
		c->events = c->events & ~WriteEvent;  // ��ȡ���ٰ�λ�룬��λ����0��λ�ö���Ϊ0
	}
}

bool isWriteEventEnable(struct Channel* c)
{
	return  c->events & WriteEvent;
}
