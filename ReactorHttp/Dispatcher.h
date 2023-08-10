#pragma once

struct EventLoop;
//io多路复用所需要用到的函数(盒子)
struct Dispatcher
{
	//init
	void* (*init)();
	//添加
	int (*add)(struct Channel* channel,struct EventLoop* evLoop);  //通过文件描述符，和数据的内存
	//删除
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	//修改
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	//事件检测
	int (*dispatch)(struct EventLoop* evLoop,int timeout); //epoll的数据内存，和超时时长
	//清除数据
	int (*clear)(struct EventLoop* evLoop );
};