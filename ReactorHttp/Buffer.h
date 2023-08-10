#pragma once

struct Buffer
{
	//ָ���ڴ��ָ��
	char* data;
	int capacity;  //�ڴ�Ĵ�С
	int readPos;  //���ڴ�ĵ�ַ
	int writePos;  //д�ڴ�ĵ�ַq
};

//��ʼ��
struct Buffer* bufferInit(int size);
//�����ڴ�
void bufferDestory(struct Buffer* buf);
//ȷ���ڴ湻д
void bufferExtendRoom(struct Buffer* buf, int size);
//��ȡʣ���д���ڴ��С
int bufferWriteSize(struct Buffer* buf);
//��ȡʣ��ɶ����ڴ��С
int bufferReadSize(struct Buffer* buf);
//�ֶ���buffer����д����
int bufferAppendData(struct Buffer* buf,const char* str,int size);
//buffer�����׽�������
int bufferSocketRead(struct Buffer* buf, int cfd);
//����"\r\n"ȡ��һ���ַ���(ע�⣬�ǽ�����ַ)
char* bufferFindCRLF(struct Buffer* buf);
//��������
int bufferSendData(struct Buffer* buffer, int cfd);