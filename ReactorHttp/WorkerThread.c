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
	//�������߳�,�������߳�ʵ����һ����Ӧ��ģ��
	pthread_creat(&thread->threadID, NULL, subThreadRunning, thread);
	//����һ������̣߳�ȷ�����̵߳ķ�Ӧ��ģ�ʹ�����ϣ�
	pthread_mutex_lock(&thread->mutex);  //��Ϊ���̺߳����̶߳����ʵ��˷�Ӧ��ģ�ͣ����Լ��ϻ�����
	while (thread->ev == NULL)
	{
		pthread_cond_wait(&thread->cond, &thread->mutex); //�������߳�
	}
	pthread_mutex_unlock(&thread->mutex);
}
void* subThreadRunning(void* arg)
{
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	//��Ϊ���̺߳����̶߳����ʵ��˷�Ӧ��ģ�ͣ����Լ��ϻ�����
	pthread_mutex_lock(&thread->mutex);
	thread->ev = eventLoopInitEx(thread->name);
	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);  //�������
	eventLoopRun(thread->ev);
	return NULL;
}
