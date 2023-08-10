#pragma once
#include <stdbool.h>

typedef int(*handleFunc)(void* arg);

enum FDEvent
{
	ReadEnent = 0x01,
	WriteEvent = 0x02
};
struct Channel
{
	int fd; //�ļ�������
	int events; //�����¼�����/д��
	handleFunc readCallback;  //���¼�����
	handleFunc writeCallback; //д�¼�
	handleFunc destroyCallback; //
	void* arg;  //�ص������Ĳ���
};

//��ʼ��һ��Channel
struct Channel* initChannel(int fd, int events, handleFunc read, handleFunc write, handleFunc destroy, void* arg);
//�޸�fd��д�¼������ or ����⣩
void writeEventEnable(struct Channel* c, bool flag);
//�ж��ļ��������Ƿ�����д�¼�
bool isWriteEventEnable(struct Channel* c);