#include "WorkerThread.h"
#include <stdio.h>

int WorkerThreadInit(struct WorkerThread* thread, int index)
{
	thread->ev = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "SubThread-%d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}

void* subThreadRunning(void* arg);
void workerThreadRun(struct WorkerThread* thread)
{
	//创建子线程,并给子线程实例化一个反应堆模型
	pthread_creat(&thread->threadID, NULL, subThreadRunning, thread);
	//阻塞一会儿主线程，确保子线程的反应堆模型创建完毕！
	pthread_mutex_lock(&thread->mutex);  //因为主线程和子线程都访问到了反应堆模型，所以加上互斥锁
	while (thread->ev == NULL)
	{
		pthread_cond_wait(&thread->cond, &thread->mutex); //阻塞主线程
	}
	pthread_mutex_unlock(&thread->mutex);
}
void* subThreadRunning(void* arg)
{
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	//因为主线程和子线程都访问到了反应堆模型，所以加上互斥锁
	pthread_mutex_lock(&thread->mutex);
	thread->ev = eventLoopInitEx(thread->name);
	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);  //解除阻塞
	eventLoopRun(thread->ev);
	return NULL;
}
