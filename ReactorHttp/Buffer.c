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
	//1.�ڴ��㹻��
	if (bufferWriteSize(buf) >= size)
	{
		return;
	}
	//2.�ڴ湻�ã������ڴ治������Ҫ�ڴ�ϲ�
	else if (buf->readPos + bufferWriteSize(buf) >= size)
	{
		//�õ�δ���Ĵ�С
		int read = bufferReadSize(buf);
		//�ƶ�λ��
		memcpy(buf->data, buf->data + buf->readPos, read);
		//����λ��
		buf->readPos = 0;
		buf->writePos = read;
	}
	//3.�ڴ治����
	else {
		//����
		void* temp = realloc(buf->data, buf->capacity + size);
		if (temp == NULL)
		{
			return;
		}
		else
		{
			memset(temp + buf->capacity,0,size); //��ʼ���µ��ڴ�
			//��������
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
	//ȷ���ڴ湻д
	bufferExtendRoom(buf, size);
	memcpy(buf->data + buf->writePos, str, size);
	buf->writePos += size;
	return 0;
}

int bufferSocketRead(struct Buffer* buf, int cfd)
{
	//ͨ��readv ��������
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
		//����ʧ��
		return -1;
	}
	else if (result <= writeable)
	{
		//buf�ڴ��㹻д��
		buf->writePos += result;
	}
	else {
		//buf�ڴ治��д��
		//��Ϊbuf�Ѿ���д��������writePos
		buf->writePos = buf->capacity;
		//�ֶ�������Ļ������е�����д��buf����Ϊд��ʱ���Զ�����,����ֻ�õ���д������ˣ�
		bufferAppendData(buf, tmp, result - writeable);
	}
	free(tmp);
	return result;
}

char* bufferFindCRLF(struct Buffer* buf)
{
	// strstr -->���ַ�����ƥ�����ַ��� (����\0����)
	// memmem -->�����ݿ���ƥ��С���ݿ� ��ָ����С��
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
