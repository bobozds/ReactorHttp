#pragma once
#define headerSize 16
#include <stdio.h>

enum HttpStatusCode
{
	Unknown=0,
	OK=200, //�ɹ�
	MovedPermantly=301, //�����ض���
	MovedTemporarily=302, //��ʱ�ض���
	BadRequest=400, //�ͻ��˷�����Ǵ�������
	NodFound=404  //�ļ�������
};

//������Ӧͷ�ṹ��
struct ResponseHeader
{
	char key[32];
	char vaule[128];
};

typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int cfd);
//Http��Ӧ�ṹ��
struct HttpResponse
{
	enum HttpStatusCode statusCode;
	char statusMsg[128];
	char fileName[128];
	//��Ӧͷ-��ֵ��
	struct ResponseHeader* headers;
	int headerNum;
	//�����ļ�/Ŀ¼�ĺ���ָ�룬��������������ļ�/Ŀ¼�ͱ����ͳ�ȥ��
	responseBody sendDataFunc;

};

//��ʼ��
struct HttpResponse* HttpResponseInit();
//����
void httpResponseDestory(struct HttpResponse* response); 
//�����Ӧͷ
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* vaule);
//��֯http��Ӧ����
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int cfd);