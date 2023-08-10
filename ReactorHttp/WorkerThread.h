#pragma once
#include <pthread.h>
#include "EventLoop.h"

//
struct WorkerThread
{
	pthread_t threadID;  //线程id
	char* name;  //线程名字
	pthread_mutex_t mutex;  //线程互斥锁
	pthread_cond_t cond; //条件变量
	struct EventLoop* ev; //反应堆模型
};

//初始化
int WorkerThreadInit(struct WorkerThread* thread,int index);
//主线程创建子线程
void workerThreadRun(struct WorkerThread* thread);
