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
	//1.创建监听的fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return NULL;
	}
	//2.设置端口复用(当主动断开连接的时候，主机会等待2MSL的时间（约1分钟），过后会释放掉端口，允许其他进程使用)
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
		perror("socket");
		return NULL;
	}
	//3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;  //网络协议
	addr.sin_port = htons(port); //端口号，注意：转成网络字节序
	addr.sin_addr.s_addr = INADDR_ANY; //ip地址，自动获取,名字叫“0地址”
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return NULL;
	}
	//4.设置监听
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

//和客户端建立连接
int acceptConnection(void* arg)
{
	struct TcpServer* server = (struct TcpServer*)arg;
	//建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	//从线程池里面取出子线程的反应堆，去处理cfd
	struct EventLoop* ev = takeWorkerEventLoop(server->pool);
	//运行tcpConnection(就是将cfd挂在树上了)
	tcpConnectionInit(cfd, ev);
	return 0;
}
void tcpServerRun(struct TcpServer* server)
{
	//启动线程池
	threadPoolRun(server->pool);
	//启动反应堆模型
	evenLoopRun(server->mainLoop);
	//包装lfd
	struct Channel* channel = initChannel(server->listener->lfd, ReadEnent, acceptConnection,NULL, NULL, server);
	//添加监听文件描述符的检测至主反应堆
	eventLoopAddTask(server->mainLoop,channel,ADD);
}
