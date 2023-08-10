#pragma once

struct Buffer
{
	//指向内存的指针
	char* data;
	int capacity;  //内存的大小
	int readPos;  //读内存的地址
	int writePos;  //写内存的地址q
};

//初始化
struct Buffer* bufferInit(int size);
//销毁内存
void bufferDestory(struct Buffer* buf);
//确保内存够写
void bufferExtendRoom(struct Buffer* buf, int size);
//获取剩余可写的内存大小
int bufferWriteSize(struct Buffer* buf);
//获取剩余可读的内存大小
int bufferReadSize(struct Buffer* buf);
//手动往buffer里面写数据
int bufferAppendData(struct Buffer* buf,const char* str,int size);
//buffer接收套接字数据
int bufferSocketRead(struct Buffer* buf, int cfd);
//根据"\r\n"取出一行字符串(注意，是结束地址)
char* bufferFindCRLF(struct Buffer* buf);
//发送数据
int bufferSendData(struct Buffer* buffer, int cfd);