#pragma once
#include "Buffer.h"
#include <stdbool.h>


//��ֵ�Խṹ��
struct Key_Value
{
	char* key;
	char* value;
};

enum HttpRequestState
{
	PaseReqLine,  //���ڽ���������
	PaseReqHeaders,  //���ڽ�������ͷ
	PaseReqBody,  //���ڽ�����������
	PaseReqDone  ////�������
};
//Http����ṹ��
struct HttpRequest
{
	char* method;  //������
	char* url;
	char* version;  //httpЭ��İ汾
	struct Key_Value* reqHeaders; //��ֵ������
	int reqHeadersNum; //��ֵ������Ĵ�С
	enum HttpRequestState curState;
};

//��ʼ��Http�ṹ��
struct HttpRequest* httpRequestInit();
//���� ����Ϊ�϶�����ͨ�ţ������һ�ε��ڴ棩
void httpRequstReset(struct HttpRequest* req);
//����
void httpRequstDestory(struct HttpRequest* req);
//�Լ�ֵ�Ե�Add����
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
//�Լ�ֵ�Ե�get����
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
//����������
bool parseHttpRequestLine(struct HttpRequest* request,struct Buffer* buf);
//��������ͷ (ֻ����һ��)
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* buf);
//�ܽ���
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* buf,
						struct HttpResponse* response,struct Buffer* sendBuf,int cfd);
//����http����
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
//�����ַ���(������ı�ת�������)
void decodeMsg(char* to, char* from);
//�����ļ���׺
const char* getFileType(const char* name);
//�����ļ�����(���ļ���һ�㷢һ��)(�����׼�����·��)
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
//����Ŀ¼���� (����һ����ҳ)
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);