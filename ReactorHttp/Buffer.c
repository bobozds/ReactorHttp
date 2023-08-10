#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>

struct Buffer* bufferInit(int size)
{
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
	buffer->data = (char*)malloc(size);
	buffer->capacity = size;
	buffer->readPos = buffer->writePos = 0;
	memset(buffer->data, 0, size);
	return buffer;
}

void bufferDestory(struct Buffer* buf)
{
	if (buf != NULL && buf->data != NULL)
	{
		free(buf->data);
	}
	free(buf);
}

void bufferExtendRoom(struct Buffer* buf, int size)
{
	//1.内存足够用
	if (bufferWriteSize(buf) >= size)
	{
		return;
	}
	//2.内存够用，但是内存不连续，要内存合并
	else if (buf->readPos + bufferWriteSize(buf) >= size)
	{
		//得到未读的大小
		int read = bufferReadSize(buf);
		//移动位置
		memcpy(buf->data, buf->data + buf->readPos, read);
		//更新位置
		buf->readPos = 0;
		buf->writePos = read;
	}
	//3.内存不够用
	else {
		//扩容
		void* temp = realloc(buf->data, buf->capacity + size);
		if (temp == NULL)
		{
			return;
		}
		else
		{
			memset(temp + buf->capacity,0,size); //初始化新的内存
			//更新数据
			buf->data = temp;
			buf->capacity += size;
		}
	}
}

int bufferWriteSize(struct Buffer* buf)
{
	return buf->capacity - buf->writePos;
}


int bufferReadSize(struct Buffer* buf)
{
	return buf->writePos - buf->readPos;
}

int bufferAppendData(struct Buffer* buf, const char* str, int size)
{
	if (buf == NULL || str == NULL || size <= 0)
		return -1;
	//确保内存够写
	bufferExtendRoom(buf, size);
	memcpy(buf->data + buf->writePos, str, size);
	buf->writePos += size;
	return 0;
}

int bufferSocketRead(struct Buffer* buf, int cfd)
{
	//通过readv 接收数据
	struct iovec vec[2];
	int writeable = bufferWriteSize(buf);
	vec[0].iov_base = buf->data + buf->writePos;
	vec[0].iov_len = writeable; 
	char* tmp = (char*)malloc(40960);
	vec[1].iov_base = tmp;
	vec[1].iov_len = 40960;
	int result = readv(cfd, vec, 2);
	if (result == -1)
	{
		//调用失败
		return -1;
	}
	else if (result <= writeable)
	{
		//buf内存足够写入
		buf->writePos += result;
	}
	else {
		//buf内存不够写入
		//因为buf已经被写满，更新writePos
		buf->writePos = buf->capacity;
		//手动将另外的缓冲区中的数据写入buf（因为写入时会自动扩容,所以只用调用写入就行了）
		bufferAppendData(buf, tmp, result - writeable);
	}
	free(tmp);
	return result;
}

char* bufferFindCRLF(struct Buffer* buf)
{
	// strstr -->大字符串中匹配子字符串 (遇到\0结束)
	// memmem -->大数据块中匹配小数据块 （指定大小）
	char* ptr = memmem(buf->data + buf->readPos, bufferReadSize(buf), "\r\n", 2);
	return ptr;
}

int bufferSendData(struct Buffer* buffer, int cfd)
{
	int reb = bufferReadSize(buffer);
	if (reb > 0)
	{
		int count = send(cfd, buffer->data + buffer->readPos, reb, 0);
		if (count > 0)
		{
			buffer->readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
