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

//���ļ�������д����
void taskWakeup(struct EventLoop* ev)
{
	const char* msg = "666";
	write(ev->socketPair[0], msg, strlen(msg));
}
//������
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
	ev->dispatcher = &EpollDispatcher;  //ָ��Ϊepoll
	ev->dispatcherData = ev->dispatcher->init(); //��Ҫepoll�ĵ�ַ����Ҫʹ��epoll�ĳ�ʼ������
	//��ʼ������
	ev->head = NULL;
	ev->tail = NULL;
	ev->map = initChannelMap(128);
	//��ʼ��������
	pthread_mutex_init(&ev->mutex, NULL);
	//��ʼ���ֶ��ļ�������
	int ret = socketpair(AF_UNIX,SOCK_STREAM,0,ev->socketPair);
	if (ret == -1)
	{
		perror("socketpair error..");
		exit(0);
	}
	//ָ������ ev->socketPair[0]�������� , 1��������
	//��װev->socketPair[1]
	struct Channel* channel = initChannel(ev->socketPair[1],ReadEnent,
		readLocalMessage,NULL,NULL,ev);
	//ͨ����������м����񣬽�channel�����map
	eventLoopAddTask(ev,channel,ADD);

	return ev;
}


int eventLoopRun(struct EventLoop* ev)
{
	assert(ev != NULL);
	//�ó��¼�ģ���б�
	struct Dispatcher* dispatcher = ev->dispatcher;
	if (ev->threadID != pthread_self())
	{
		perror("threadID error");
		return -1;
	}
	while (!ev->isQuit)
	{
		//ѭ�������¼�(epoll_wait)�����������ʹ����д�¼�������ĵĲ��֣�
		dispatcher->dispatch(ev, 2);
		//ѭ�������ļ��������������б���ɾ�ģ�
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
	//ͨ��fd��ȡ��channel
	struct Channel* channel = ev->map->list[fd];
	assert(channel->fd == fd);
	//Ȼ��channel��Ӧ���¼�����
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
	//�������������߳̿��ܻ���channelMap��������ļ�������
	pthread_mutex_lock(&ev->mutex);
	//�����½ڵ�
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
	//����
	pthread_mutex_unlock(&ev->mutex);
	//����ڵ�
	//ϸ�ڣ���������̣߳����������̴߳���������У��������߳�����
	if (ev->threadID == pthread_self())
	{
		//��ǰ�����߳�
		//��������
		eventLoopProcessTask(ev);
	}
	else
	{
		//��ǰ�����߳� -- �������̣߳������ȥ����������������������
		//�������1.���߳����ڹ���
		
		//2.���̱߳�������epoll_wait�����޷��أ�
		//����������ֶ���һ���ļ�������������һ���ļ�������������
		taskWakeup(ev); //ע�⣺��������֮���ǽ����epoll_wait�������������������������б��������epoll_wait��������Ժ��ִ�������б���ϸ��eventLoopRun����

	}
	return 0;
}

int eventLoopProcessTask(struct EventLoop* ev)
{
	pthread_mutex_lock(&ev->mutex); //����
	struct ChannelElement* head = ev->head;
	while (head != NULL)
	{
		struct Channel* channel = head->channel;
		if (head->type == ADD)
		{
			//���
			TaskAdd(ev, channel);
		}
		else if (head->type == DELETE)
		{
			//ɾ��
			TaskRemove(ev, channel);
		}
		else if (head->type == MODIFY)
		{
			//�޸�
			TaskModfiy(ev, channel);
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	ev->head = ev->tail = NULL;
	pthread_mutex_unlock(&ev->mutex); //����
	return 0;
}

int TaskAdd(struct EventLoop* ev, struct Channel* channel)
{
	int fd = channel->fd;
	struct ChannelMap* map = ev->map;
	if (fd >= map->size)
	{
		//�ռ䲻��������
		if (!makeMapRoom(map, fd, sizeof(struct Channel*)));
		{
			return -1;
		}
	}
	if(map->list[fd] == NULL)
	{
		map->list[fd] = channel; //channel�����map
		ev->dispatcher->add(channel, ev); //���ļ���������������
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
	//��ʬ��
	destroyChannel(ev, channel);
	int ret =  ev->dispatcher->remove(channel, ev); //������ɾ��
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
	int ret =  ev->dispatcher->modify(channel, ev); //���ļ���������������
	return ret;
}

int destroyChannel(struct EventLoop* ev, struct Channel* channel)
{
	struct ChannelMap* map = ev->map;
	if (map->list[channel->fd] != NULL)
	{
		map->list[channel->fd] = NULL;
	}
	//�ر�fd
	close(channel->fd);
	//�ͷ�channel
	free(channel);
	return 0;
}
