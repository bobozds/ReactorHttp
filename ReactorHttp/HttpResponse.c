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
	//��ʼ������
	bzero(res->headers, size);
	bzero(res->statusMsg, sizeof(res->statusMsg));
	bzero(res->fileName,sizeof(res->fileName) );
	//����ָ��
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
	//״̬��
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
	int len = strlen(tmp);
	bufferAppendData(sendBuf, tmp,len);
	//��Ӧͷ
	for (int i = 0; i < response->headerNum; i++)
	{
		sprintf(tmp, "%s: %s", response->headers[response->headerNum].key, response->headers[response->headerNum].vaule);
	}
	len = strlen(tmp);
	bufferAppendData(sendBuf, tmp, len);
	//����
	bufferAppendData(sendBuf, "\r\n", 2);
	
#ifndef DEBUG
	//��buffer(��ʱ������״̬�С���Ӧͷ������)���͸��ͻ���
	bufferSendData(sendBuf, cfd);
#endif // DEBUG
	//������ûظ���������Ŀ¼/�ļ����͸��ͻ���
	response->sendDataFunc(response->fileName, sendBuf, cfd);
}


