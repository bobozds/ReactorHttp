#include "EventLoop.h"
#include "ChannelMap.h"
#include <stdio.h>
#include <assert.h>
#include "Channel.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct EventLoop* eventLoopInit()
{
	return eventLoopInitEx(NULL);
}

//往文件描述符写数据
void taskWakeup(struct EventLoop* ev)
{
	const char* msg = "666";
	write(ev->socketPair[0], msg, strlen(msg));
}
//读数据
int readLocalMessage(void* arg)
{
	struct EventLoop* ev = (struct EventLoop*)arg;
	char buf[256];
	read(ev->socketPair[1], buf, sizeof(buf));
}
struct EventLoop* eventLoopInitEx(const char* Name)
{
	struct EventLoop* ev = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	ev->isQuit = false;
	ev->threadID = pthread_self();
	ev->threadName = Name == NULL? "MainThread" : Name;
	ev->dispatcher = &EpollDispatcher;  //指定为epoll
	ev->dispatcherData = ev->dispatcher->init(); //想要epoll的地址，就要使用epoll的初始化函数
	//初始化链表
	ev->head = NULL;
	ev->tail = NULL;
	ev->map = initChannelMap(128);
	//初始化互斥锁
	pthread_mutex_init(&ev->mutex, NULL);
	//初始化手动文件描述符
	int ret = socketpair(AF_UNIX,SOCK_STREAM,0,ev->socketPair);
	if (ret == -1)
	{
		perror("socketpair error..");
		exit(0);
	}
	//指定规则： ev->socketPair[0]发送数据 , 1接收数据
	//包装ev->socketPair[1]
	struct Channel* channel = initChannel(ev->socketPair[1],ReadEnent,
		readLocalMessage,NULL,NULL,ev);
	//通过给任务队列加任务，将channel添加至map
	eventLoopAddTask(ev,channel,ADD);

	return ev;
}


int eventLoopRun(struct EventLoop* ev)
{
	assert(ev != NULL);
	//拿出事件模型列表
	struct Dispatcher* dispatcher = ev->dispatcher;
	if (ev->threadID != pthread_self())
	{
		perror("threadID error");
		return -1;
	}
	while (!ev->isQuit)
	{
		//循环处理事件(epoll_wait)（包括监听和处理读写事件，最核心的部分）
		dispatcher->dispatch(ev, 2);
		//循环处理文件描述符的任务列表（增删改）
		eventLoopProcessTask(ev);
	}
	return 0;
}

int eventActivate(struct EventLoop* ev, int fd, int event)
{
	if (fd < 0 || ev==NULL)
	{
		return -1;
	}
	//通过fd来取出channel
	struct Channel* channel = ev->map->list[fd];
	assert(channel->fd == fd);
	//然后将channel对应的事件触发
	if (event & ReadEnent && channel->readCallback != NULL)
	{
		channel->readCallback(channel->arg);
	}
	if (event & WriteEvent && channel->writeCallback != NULL)
	{
		channel->writeCallback(channel->arg);
	}
	return 0;
}

int eventLoopAddTask(struct EventLoop* ev, struct Channel* channel, int type)
{
	//上锁，由于主线程可能会往channelMap里面添加文件描述符
	pthread_mutex_lock(&ev->mutex);
	//创建新节点
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	node->channel = channel;
	node->type = type;
	node->next = NULL;
	if (ev->head == NULL)
	{
		ev->head = ev->tail = node;
	}
	else {
		ev->tail->next = node;
		ev->tail = node;
	}
	//解锁
	pthread_mutex_unlock(&ev->mutex);
	//处理节点
	//细节：如果是主线程，不能让主线程处理任务队列，交给子线程来做
	if (ev->threadID == pthread_self())
	{
		//当前是子线程
		//遍历链表
		eventLoopProcessTask(ev);
	}
	else
	{
		//当前是主线程 -- 告诉子线程，你可以去处理任务队列里面的任务了
		//两种情况1.子线程正在工作
		
		//2.子线程被阻塞（epoll_wait函数无返回）
		//解决方法：手动用一个文件描述符向另外一个文件描述符发数据
		taskWakeup(ev); //注意：当调用了之后，是解除了epoll_wait的阻塞！！！！！不是任务列表的阻塞，epoll_wait阻塞解除以后会执行任务列表，详细见eventLoopRun！！

	}
	return 0;
}

int eventLoopProcessTask(struct EventLoop* ev)
{
	pthread_mutex_lock(&ev->mutex); //上锁
	struct ChannelElement* head = ev->head;
	while (head != NULL)
	{
		struct Channel* channel = head->channel;
		if (head->type == ADD)
		{
			//添加
			TaskAdd(ev, channel);
		}
		else if (head->type == DELETE)
		{
			//删除
			TaskRemove(ev, channel);
		}
		else if (head->type == MODIFY)
		{
			//修改
			TaskModfiy(ev, channel);
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	ev->head = ev->tail = NULL;
	pthread_mutex_unlock(&ev->mutex); //解锁
	return 0;
}

int TaskAdd(struct EventLoop* ev, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* map = ev->map;
	if (fd >= map->size)
	{
		//空间不够，扩容
		if (!makeMapRoom(map, fd, sizeof(struct Channel*)));
		{
			return -1;
		}
	}
	if(map->list[fd] == NULL)
	{
		map->list[fd] = channel; //channel添加至map
		ev->dispatcher->add(channel, ev); //将文件描述符挂在树上
	}
	return 0;
}

int TaskRemove(struct EventLoop* ev, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* map = ev->map;
	if (fd >= map->size)
	{
		return -1;
	}
	//毁尸灭迹
	destroyChannel(ev, channel);
	int ret =  ev->dispatcher->remove(channel, ev); //从树上删除
	return ret;
}


int TaskModfiy(struct EventLoop* ev, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* map = ev->map;
	if (fd >= map->size || map->list[fd] == NULL)
	{

		return -1;
	}
	int ret =  ev->dispatcher->modify(channel, ev); //将文件描述符挂在树上
	return ret;
}

int destroyChannel(struct EventLoop* ev, struct Channel* channel)
{
	struct ChannelMap* map = ev->map;
	if (map->list[channel->fd] != NULL)
	{
		map->list[channel->fd] = NULL;
	}
	//关闭fd
	close(channel->fd);
	//释放channel
	free(channel);
	return 0;
}
