#pragma once

struct EventLoop;
//io��·��������Ҫ�õ��ĺ���(����)
struct Dispatcher
{
	//init
	void* (*init)();
	//���
	int (*add)(struct Channel* channel,struct EventLoop* evLoop);  //ͨ���ļ��������������ݵ��ڴ�
	//ɾ��
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	//�޸�
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	//�¼����
	int (*dispatch)(struct EventLoop* evLoop,int timeout); //epoll�������ڴ棬�ͳ�ʱʱ��
	//�������
	int (*clear)(struct EventLoop* evLoop );
};