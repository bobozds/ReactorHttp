#pragma once
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <stdbool.h>
#include <pthread.h>

extern struct Dispatcher EpollDispatcher;
enum ElemType{ADD,DELETE,MODIFY};
//一个channel链表节点类型
struct ChannelElement 
{
	int type;  //对channel的操作（增删改）
	struct Channel* channel;
	struct ChannelElement* next;
};

struct Dispatcher;
//这是一个循环的反应堆模型
struct EventLoop
{
	bool isQuit; //循环开关
	struct Dispatcher* dispatcher; //io多路复用所需要用到的函数
	void* dispatcherData;  //io多路复用需要的内存 （epool树内存，文件描述符内存等等）
	//任务队列
	struct ChannelElement* head; //头结点
	struct ChannelElement* tail; //尾节点
	//文件描述符集合
	struct ChannelMap* map;
	//线程ID，name
	pthread_t threadID;
	char* threadName;
	//互斥锁
	pthread_mutex_t mutex;
	int socketPair[2]; //储存手动文件描述符
};

//初始化主线程的反应堆模型
struct EventLoop* eventLoopInit(); 
//初始化子线程的反应堆模型
struct EventLoop* eventLoopInitEx(const char* Name); 
//启动反应堆，传进来实例是什么，就把什么启动起来 （主要就是启动epoll_wait）
int eventLoopRun(struct EventLoop* ev);
//处理被激活的文件描述符
int eventActivate(struct EventLoop* ev, int fd, int event);
//添加任务到任务队列
int eventLoopAddTask(struct EventLoop* ev, struct Channel* channel, int type);
//遍历处理任务队列
int eventLoopProcessTask(struct EventLoop* ev);
//任务队列：添加子函数(将channel添加进map)，并将它挂在树上
int TaskAdd(struct EventLoop* ev, struct Channel* channel);
//任务队列：删除子函数(将map中的channel删除)
int TaskRemove(struct EventLoop* ev, struct Channel* channel);
//任务队列：修改子函数(修改map中的channel)
int TaskModfiy(struct EventLoop* ev, struct Channel* channel);
//释放channel
int destroyChannel(struct EventLoop* ev, struct Channel* channel);