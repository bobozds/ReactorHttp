#include "ThreadPool.h"
#include <stdio.h>
#include <assert.h>
#include "WorkerThread.h"

struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)
{
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	pool->index = 0;
	pool->isStart = false;
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct WorkThread*)malloc(sizeof(struct WorkThread*) * count);
	return pool;
}

int threadPoolRun(struct ThreadPool* pool)
{
	//�ж��̳߳��Ƿ��Ѿ�����
	if (pool == NULL || pool->isStart == true)
	{
		return -1;
	}
	//�ж�ʹ������������ǲ������߳�
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum)
	{
		for (int i = 0; i < pool->threadNum; i++)
		{
			WorkThreadInit(&pool->workerThreads[i],i);
			WorkThreadRun(&pool->workerThreads[i]);
		}
	}

	return 0;
}

struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
	assert(pool->isStart);
	if (pool->mainLoop->threadID != pthread_self())
	{
		exit(0);
	}
	//���̳߳�������һ�����̣߳��������ó���Ӧ��
	struct EventLoop* evLoop = pool->mainLoop;
	if (pool->threadNum > 0)
	{
		evLoop = pool->workerThreads[pool->index].ev;
		pool->index++;
		pool->index = ++pool->index % pool->threadNum;
	}
	return evLoop;
}


