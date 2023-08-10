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
		c->events |= WriteEvent;  //按位或，按位或会把1的位置变成1  
	}
	else {
		c->events = c->events & ~WriteEvent;  // 先取反再按位与，按位与会把0的位置都变为0
	}
}

bool isWriteEventEnable(struct Channel* c)
{
	return  c->events & WriteEvent;
}
