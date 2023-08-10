#pragma once
#include "Buffer.h"
#include <stdbool.h>


//键值对结构体
struct Key_Value
{
	char* key;
	char* value;
};

enum HttpRequestState
{
	PaseReqLine,  //正在解析请求行
	PaseReqHeaders,  //正在解析请求头
	PaseReqBody,  //正在解析请求数据
	PaseReqDone  ////解析完毕
};
//Http请求结构体
struct HttpRequest
{
	char* method;  //请求行
	char* url;
	char* version;  //http协议的版本
	struct Key_Value* reqHeaders; //键值对数组
	int reqHeadersNum; //键值对数组的大小
	enum HttpRequestState curState;
};

//初始化Http结构体
struct HttpRequest* httpRequestInit();
//重置 （因为肯定会多次通信，清除上一次的内存）
void httpRequstReset(struct HttpRequest* req);
//销毁
void httpRequstDestory(struct HttpRequest* req);
//对键值对的Add方法
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
//对键值对的get方法
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
//解析请求行
bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* buf);
//解析请求头 (只处理一行)
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* buf);
//总解析
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* buf,
						struct HttpResponse* response,struct Buffer* sendBuf,int cfd);
//处理http请求
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
//解码字符串(解决中文被转码的问题)
void decodeMsg(char* to, char* from);
//解析文件后缀
const char* getFileType(const char* name);
//发送文件函数(将文件读一点发一点)(传入标准的相对路径)
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
//发送目录函数 (发送一个网页)
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);