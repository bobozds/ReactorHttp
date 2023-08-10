#pragma once
#include <pthread.h>
#include "EventLoop.h"

//
struct WorkerThread
{
	pthread_t threadID;  //�߳�id
	char* name;  //�߳�����
	pthread_mutex_t mutex;  //�̻߳�����
	pthread_cond_t cond; //��������
	struct EventLoop* ev; //��Ӧ��ģ��
};

//��ʼ��
int WorkerThreadInit(struct WorkerThread* thread,int index);
//���̴߳������߳�
void workerThreadRun(struct WorkerThread* thread);
