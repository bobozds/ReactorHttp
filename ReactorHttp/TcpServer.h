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
	
	struct EventLoop* mainLoop; //����Ӧ��
	struct ThreadPool* pool; //�̳߳�
	int threadNum;  //�̳߳������̵߳ĸ���
	struct Listener* listener; 
};

//��ʼ�������ļ�������
struct Listener* listenterInit(unsigned short port);
//��ʼ��
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
//����������
void tcpServerRun(struct TcpServer* server);