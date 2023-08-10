#include "Dispatcher.h"
#include "Channel.h"
#include "EventLoop.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

//结构体数组的容量
#define Max 520
struct EpollData
{
	int epfd;  //epoll树的根节点
	struct epoll_event* events; //epoll_wait函数的传出参数
};
static void* epollinit();
static int epolladd(struct Channel* channel, struct EventLoop* evLoop);  //通过文件描述符，和数据的内存
static int epollremove(struct Channel* channel, struct EventLoop* evLoop);
static int epollmodify(struct Channel* channel, struct EventLoop* evLoop);
static int epolldispatch(struct EventLoop* evLoop, int timeout); //epoll的数据内存，和超时时长
static int epollclear(struct EventLoop* evLoop);
static int epollctl(struct Channel* channel, struct EventLoop* evLoop,int op);

struct Dispatcher EpollDispatcher = {
	epollinit,
	epolladd,
	epollremove,
	epollmodify,
	epolldispatch,
	epollclear
};

static void* epollinit()
{
	struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
	data->epfd = epoll_create(1);
	if (data->epfd == -1)
	{
		perror("epoll_create");
		exit(1);
	}
	data->events = (struct epoll_event*)calloc(Max, sizeof(struct epoll_event));
	return data;
}

static int epolladd(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollctl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1)
	{
		perror("epoll_ctl_add");
		exit(1);
	}
	return ret;
}


static int epollremove(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollctl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1)
	{
		perror("epoll_ctl_del");
		exit(1);
	}
	//通过channel释放对应的TcpConnection
	channel->destroyCallback(channel->arg);
	return ret;
}

static int epollmodify(struct Channel* channel, struct EventLoop* evLoop)
{
	int ret = epollctl(channel, evLoop, EPOLL_CTL_MOD);
	if (ret == -1)
	{
		perror("epoll_ctl_del");
		exit(1);
	}
	return ret;
}

static int epollctl(struct Channel* channel, struct EventLoop* evLoop, int op)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int events = 0;
	if (channel->events & ReadEnent)  //如果读开关开启，按位与就是检验
	{
		events |= EPOLLIN;  //那么就官方的标志位的读开关也要开启,按位或就是加上
	}
	if (channel->events & WriteEvent)
	{
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return 0;
}
static int epolldispatch(struct EventLoop* evLoop, int timeout)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	int count = epoll_wait(data->epfd, data->events, Max, timeout * 1000);
	for (int i = 0; i < count; i++)
	{
		//先拿出当前数据
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		//错误判断
		if (events & EPOLLERR || events & EPOLLHUP)  //EPOLLERR：对方断开连接  EPOLLHUP：对方断开连接后，还在通信
		{
			//删除fd
			//epollremove(channel,evLoop);
			continue;
		}
		//读数据触发
		if (events & EPOLLIN)
		{
			eventActivate(evLoop, fd, ReadEnent);
		}
		//写数据触发
		if (events & EPOLLOUT)
		{
			eventActivate(evLoop, fd, WriteEvent);
		}

	}
}
static int epollclear(struct EventLoop* evLoop)
{
	struct EpollData* data = (struct EpollData*)evLoop->dispatcherData;
	free(data->events);
	close(data->epfd);
	free(data);
	return 0;
}