#include "HttpResponse.h"
#include <string.h>
#include <stdlib.h>
#include "Buffer.h"

struct HttpResponse* HttpResponseInit()
{
	struct HttpResponse* res = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
	res->headerNum = 0;
	int size = sizeof(struct ResponseHeader) * headerSize;
	res->headers = (struct ResponseHeader*)malloc(size);
	res->statusCode = Unknown;
	//初始化数组
	bzero(res->headers, size);
	bzero(res->statusMsg, sizeof(res->statusMsg));
	bzero(res->fileName,sizeof(res->fileName) );
	//函数指针
	res->sendDataFunc = NULL;
	
	return res;

}

void httpResponseDestory(struct HttpResponse* response)
{
	if (response != NULL)
	{
		free(response->headers);
		free(response);
	}
}

void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value)
{
	if (response == NULL || key == NULL || value == NULL)
	{
		return;
	}
	strcpy(response->headers[response->headerNum].key , key);
	strcpy(response->headers[response->headerNum].vaule, value);
	response->headerNum++;
}

void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int cfd)
{
	//状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
	int len = strlen(tmp);
	bufferAppendData(sendBuf, tmp,len);
	//响应头
	for (int i = 0; i < response->headerNum; i++)
	{
		sprintf(tmp, "%s: %s", response->headers[response->headerNum].key, response->headers[response->headerNum].vaule);
	}
	len = strlen(tmp);
	bufferAppendData(sendBuf, tmp, len);
	//空行
	bufferAppendData(sendBuf, "\r\n", 2);
	
#ifndef DEBUG
	//将buffer(此时里面是状态行、响应头、空行)发送给客户端
	bufferSendData(sendBuf, cfd);
#endif // DEBUG
	//在这调用回复函数，将目录/文件发送给客户端
	response->sendDataFunc(response->fileName, sendBuf, cfd);
}


