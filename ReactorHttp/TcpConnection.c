#include "TcpConnection.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//���¼����������������ݴ洢��buffer����
int ReadClientData(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//��������
	int count = bufferSocketRead(conn->readBuf,conn->channel->fd);
	if (count > 0)
	{

		//���յ������ݣ�����http����
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, conn->channel->fd);
		if (!flag)
		{
			//����ʧ��
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			int len = strlen(errMsg);
			bufferAppendData(conn->writeBuf,errMsg,len);
		}
	}
#ifdef DEBUG  //һ����ʽ�������ݣ�ͨ��epollд�¼���ⷢ�ͣ���������Ҫ�����ݽ�����ϲŷ��ͣ����ݿ���̫��bufferװ���µ��³������
	//���д�¼�����
	writeEventEnable(conn->channel, true);  //�޸�channel�����ټ��д�¼�
	eventLoopAddTask(conn->ev, conn->channel, MODIFY);  //ˢ��epoll���ϵĵ�ǰ�ڵ��¼�
#endif // DEBUG
#ifndef DEBUG  //��ʽһ���ܶϿ����ӣ��Ͽ���û�м����
	//�Ͽ�����,���ǰ�channel��map��ɾ������fd������������
	eventLoopAddTask(conn->ev, conn->channel, DELETE);
#endif // DEBUG
	return 0;
}

//д�¼���������
int WriteClientData(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//��������
	 int count = bufferSendData(conn->readBuf, conn->channel->fd);
	 if (count > 0)
	 {
		 //ȷʵ���ͳ�ȥ��,�жϷ�������û��
		 if (bufferReadSize(conn->readBuf) == 0)
		 {
			 //��������
			 writeEventEnable(conn->channel, false);  //�޸�channel�����ټ��д�¼�
			 eventLoopAddTask(conn->ev, conn->channel, MODIFY);  //ˢ��epoll���ϵĵ�ǰ�ڵ��¼�
			 eventLoopAddTask(conn->ev, conn->channel, DELETE);  //�Ͽ����ӣ�û����Ϊʲô��

		 }
	 }
}
struct TcpConnection* tcpConnectionInit(int cfd, struct EvenLoop* ev)
{
	struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
	sprintf(conn->name,"Connection-%d", cfd);
	conn->ev = ev;
	conn->readBuf = bufferInit(10240);
	conn->writeBuf = bufferInit(10240);
	conn->channel = initChannel(cfd, ReadEnent, ReadClientData, WriteClientData, tcpConnectionDestroy, conn);
	//�������Ӧ�ѣ�����������
	eventLoopAddTask(ev, conn->channel, ADD);

	conn->request = httpRequestInit();
	conn->response = HttpResponseInit();
	return conn;
}

int tcpConnectionDestroy(void * arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	if (conn != NULL)
	{
		if (conn->readBuf && bufferReadSize(conn->readBuf) == 0 && conn->writeBuf && bufferReadSize(conn->writeBuf) == 0)
		{
			destroyChannel(conn->ev, conn->channel);
			bufferDestory(conn->readBuf);
			bufferDestory(conn->writeBuf);
			httpResponseDestory(conn->response);
			httpRequstDestory(conn->request);
			free(conn);
		}
	}
	return 0;
}
