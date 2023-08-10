#include "TcpServer.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TcpConnection.h"
#include <stdio.h>
#include <arpa/inet.h>
#include "TcpServer.h"
#include <stdlib.h>

struct Listener* listenterInit(unsigned short port)
{
	struct Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	//1.����������fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return NULL;
	}
	//2.���ö˿ڸ���(�������Ͽ����ӵ�ʱ��������ȴ�2MSL��ʱ�䣨Լ1���ӣ���������ͷŵ��˿ڣ�������������ʹ��)
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
		perror("socket");
		return NULL;
	}
	//3.��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //����Э��
	addr.sin_port = htons(port); //�˿ںţ�ע�⣺ת�������ֽ���
	addr.sin_addr.s_addr = INADDR_ANY; //ip��ַ���Զ���ȡ,���ֽС�0��ַ��
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return NULL;
	}
	//4.���ü���
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return NULL;
	}
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}

struct TcpServer* tcpServerInit(unsigned short port, int threadNum)
{
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->listener = listenterInit(port);
	tcp->mainLoop = eventLoopInit();
	tcp->pool = threadPoolInit(tcp->mainLoop,threadNum);
	tcp->threadNum = threadNum;
	return tcp;
}

//�Ϳͻ��˽�������
int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	//��������
	int cfd = accept(server->listener->lfd, NULL, NULL);
	//���̳߳�����ȡ�����̵߳ķ�Ӧ�ѣ�ȥ����cfd
	struct EventLoop* ev = takeWorkerEventLoop(server->pool);
	//����tcpConnection(���ǽ�cfd����������)
	tcpConnectionInit(cfd, ev);
	return 0;
}
void tcpServerRun(struct TcpServer* server)
{
	//�����̳߳�
	threadPoolRun(server->pool);
	//������Ӧ��ģ��
	evenLoopRun(server->mainLoop);
	//��װlfd
	struct Channel* channel = initChannel(server->listener->lfd, ReadEnent, acceptConnection,NULL, NULL, server);
	//��Ӽ����ļ��������ļ��������Ӧ��
	eventLoopAddTask(server->mainLoop,channel,ADD);
}
