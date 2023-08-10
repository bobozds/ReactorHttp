#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"

struct Listener
{
	int lfd;
	unsigned short port;
};
struct TcpServer
{
	
	struct EventLoop* mainLoop; //主反应堆
	struct ThreadPool* pool; //线程池
	int threadNum;  //线程池中子线程的个数
	struct Listener* listener; 
};

//初始化监听文件描述符
struct Listener* listenterInit(unsigned short port);
//初始化
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
//启动服务器
void tcpServerRun(struct TcpServer* server);