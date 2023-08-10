#include "TcpConnection.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//读事件触发函数，将数据存储到buffer里面
int ReadClientData(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//接收数据
	int count = bufferSocketRead(conn->readBuf,conn->channel->fd);
	if (count > 0)
	{

		//接收到了数据，进行http请求
		bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, conn->channel->fd);
		if (!flag)
		{
			//解析失败
			char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			int len = strlen(errMsg);
			bufferAppendData(conn->writeBuf,errMsg,len);
		}
	}
#ifdef DEBUG  //一、方式发送数据，通过epoll写事件检测发送，问题是需要等数据接收完毕才发送，数据可能太大buffer装不下导致程序崩溃
	//添加写事件监听
	writeEventEnable(conn->channel, true);  //修改channel，不再检测写事件
	eventLoopAddTask(conn->ev, conn->channel, MODIFY);  //刷新epoll树上的当前节点事件
#endif // DEBUG
#ifndef DEBUG  //方式一不能断开连接，断开就没有检测了
	//断开连接,就是把channel从map中删除，把fd从树上拿下来
	eventLoopAddTask(conn->ev, conn->channel, DELETE);
#endif // DEBUG
	return 0;
}

//写事件触发函数
int WriteClientData(void* arg)
{
	struct TcpConnection* conn = (struct TcpConnection*)arg;
	//发送数据
	 int count = bufferSendData(conn->readBuf, conn->channel->fd);
	 if (count > 0)
	 {
		 //确实发送出去了,判断发送完了没有
		 if (bufferReadSize(conn->readBuf) == 0)
		 {
			 //发送完了
			 writeEventEnable(conn->channel, false);  //修改channel，不再检测写事件
			 eventLoopAddTask(conn->ev, conn->channel, MODIFY);  //刷新epoll树上的当前节点事件
			 eventLoopAddTask(conn->ev, conn->channel, DELETE);  //断开连接（没明白为什么）

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
	//添加至反应堆，挂载在树上
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
