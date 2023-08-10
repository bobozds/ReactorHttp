#pragma once
#include "Buffer.h"
#include "EventLoop.h"
#include "Channel.h"
#include "HttpResponse.h"
#include "HttpRequest.h"

// #define DEBUG

struct TcpConnection
{
	struct EventLoop* ev;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	//
	struct HttpResponse* response;
	struct HttpRequest* request;
};

//≥ı ºªØ
struct TcpConnection* tcpConnectionInit(int cfd, struct EvenLoop* ev);
// Õ∑≈TcpConnection
int tcpConnectionDestroy(void* arg);