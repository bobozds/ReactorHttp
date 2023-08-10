#pragma once
#include "Dispatcher.h"
#include "ChannelMap.h"
#include <stdbool.h>
#include <pthread.h>

extern struct Dispatcher EpollDispatcher;
enum ElemType{ADD,DELETE,MODIFY};
//һ��channel����ڵ�����
struct ChannelElement 
{
	int type;  //��channel�Ĳ�������ɾ�ģ�
	struct Channel* channel;
	struct ChannelElement* next;
};

struct Dispatcher;
//����һ��ѭ���ķ�Ӧ��ģ��
struct EventLoop
{
	bool isQuit; //ѭ������
	struct Dispatcher* dispatcher; //io��·��������Ҫ�õ��ĺ���
	void* dispatcherData;  //io��·������Ҫ���ڴ� ��epool���ڴ棬�ļ��������ڴ�ȵȣ�
	//�������
	struct ChannelElement* head; //ͷ���
	struct ChannelElement* tail; //β�ڵ�
	//�ļ�����������
	struct ChannelMap* map;
	//�߳�ID��name
	pthread_t threadID;
	char* threadName;
	//������
	pthread_mutex_t mutex;
	int socketPair[2]; //�����ֶ��ļ�������
};

//��ʼ�����̵߳ķ�Ӧ��ģ��
struct EventLoop* eventLoopInit(); 
//��ʼ�����̵߳ķ�Ӧ��ģ��
struct EventLoop* eventLoopInitEx(const char* Name); 
//������Ӧ�ѣ�������ʵ����ʲô���Ͱ�ʲô�������� ����Ҫ��������epoll_wait��
int eventLoopRun(struct EventLoop* ev);
//����������ļ�������
int eventActivate(struct EventLoop* ev, int fd, int event);
//��������������
int eventLoopAddTask(struct EventLoop* ev, struct Channel* channel, int type);
//���������������
int eventLoopProcessTask(struct EventLoop* ev);
//������У�����Ӻ���(��channel��ӽ�map)����������������
int TaskAdd(struct EventLoop* ev, struct Channel* channel);
//������У�ɾ���Ӻ���(��map�е�channelɾ��)
int TaskRemove(struct EventLoop* ev, struct Channel* channel);
//������У��޸��Ӻ���(�޸�map�е�channel)
int TaskModfiy(struct EventLoop* ev, struct Channel* channel);
//�ͷ�channel
int destroyChannel(struct EventLoop* ev, struct Channel* channel);