#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdlib.h>

//线程池
struct ThreadPool
{
	struct EventLoop* mainLoop;  //只负责和客户端建立连接
	bool isStart; //线程池开关
	int threadNum; //子线程个数
	struct WorkerThread* workerThreads; //子线程集合
	int index; //当前派任务给线程的子线程编号(遍历)
};

//初始化线程池 (传入一个主反应堆，子线程个数)
struct ThreadPool* threadPoolInit(struct EventLoop* ev,int count);
//运行线程池
int threadPoolRun(struct ThreadPool* pool);
//取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);