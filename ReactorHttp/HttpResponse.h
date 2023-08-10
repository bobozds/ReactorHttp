#pragma once
#define headerSize 16
#include <stdio.h>

enum HttpStatusCode
{
	Unknown=0,
	OK=200, //成功
	MovedPermantly=301, //永久重定向
	MovedTemporarily=302, //临时重定向
	BadRequest=400, //客户端发起的是错误请求
	NodFound=404  //文件不存在
};

//定义响应头结构体
struct ResponseHeader
{
	char key[32];
	char vaule[128];
};

typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int cfd);
//Http响应结构体
struct HttpResponse
{
	enum HttpStatusCode statusCode;
	char statusMsg[128];
	char fileName[128];
	//响应头-键值对
	struct ResponseHeader* headers;
	int headerNum;
	//发送文件/目录的函数指针，调用这个函数，文件/目录就被发送出去了
	responseBody sendDataFunc;

};

//初始化
struct HttpResponse* HttpResponseInit();
//销毁
void httpResponseDestory(struct HttpResponse* response); 
//添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* vaule);
//组织http响应数据
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int cfd);