#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdlib.h>

//�̳߳�
struct ThreadPool
{
	struct EventLoop* mainLoop;  //ֻ����Ϳͻ��˽�������
	bool isStart; //�̳߳ؿ���
	int threadNum; //���̸߳���
	struct WorkerThread* workerThreads; //���̼߳���
	int index; //��ǰ��������̵߳����̱߳��(����)
};

//��ʼ���̳߳� (����һ������Ӧ�ѣ����̸߳���)
struct ThreadPool* threadPoolInit(struct EventLoop* ev,int count);
//�����̳߳�
int threadPoolRun(struct ThreadPool* pool);
//ȡ���̳߳��е�ĳ�����̵߳ķ�Ӧ��ʵ��
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);