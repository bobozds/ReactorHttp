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
	int fd; //文件描述符
	int events; //处理事件（读/写）
	handleFunc readCallback;  //读事件函数
	handleFunc writeCallback; //写事件
	handleFunc destroyCallback; //
	void* arg;  //回调函数的参数
};

//初始化一个Channel
struct Channel* initChannel(int fd, int events, handleFunc read, handleFunc write, handleFunc destroy, void* arg);
//修改fd的写事件（检测 or 不检测）
void writeEventEnable(struct Channel* c, bool flag);
//判断文件描述符是否开启了写事件
bool isWriteEventEnable(struct Channel* c);