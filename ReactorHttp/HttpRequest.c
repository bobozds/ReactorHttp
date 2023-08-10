#include "HttpRequest.h"
#include "HttpResponse.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>

#define HeaderSize 12
struct HttpRequest* httpRequestInit()
{
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	request->curState = PaseReqLine;
	request->method = NULL;
	request->reqHeaders = (struct Key_Value*)malloc(sizeof(struct Key_Value*) * HeaderSize);
	request->reqHeadersNum = 0;
	request->url = NULL;
	request->version = NULL;
	return request;
}

void httpRequstReset(struct HttpRequest* req)
{
	req->curState = PaseReqLine;
	free(req->method);
	free(req->url);
	free(req->version);
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	//�Ѽ�ֵ���ͷŵ�
	if (req->reqHeaders != NULL)
	{
		for (int i = 0; i < req->reqHeadersNum; i++)
		{
			free(req->reqHeaders[i].key);
			free(req->reqHeaders[i].value);
		}
	}
	req->reqHeadersNum = 0;
}

void httpRequstDestory(struct HttpRequest* req)
{
	if (req != NULL)
	{
		httpRequstReset(req);
		//�Ѽ�ֵ�������ͷŵ�
		if (req->reqHeaders != NULL)
		{
			free(req->reqHeaders);
		}
		free(req);
	}
}

void httpRequestAddHeader(struct HttpRequest* req, const char* key, const char* value)
{
	if (req != NULL && req->reqHeaders != NULL)
	{
		req->reqHeaders[req->reqHeadersNum].key = key;
		req->reqHeaders[req->reqHeadersNum].value = value;
		req->reqHeadersNum++;
	}
	
}

char* httpRequestGetHeader(struct HttpRequest* req, const char* key)
{
	if(req != NULL && req->reqHeaders != NULL)
	for (int i = 0; i < req->reqHeadersNum; i++)
	{
		if (strncasecmp(req->reqHeaders[i].key,key,strlen(key)) ==0 )
		{
			return req->reqHeaders[i].value;
		}
	}
	return "";
}

bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* buf)
{
	//�����н�����ַ
	char* end = bufferFindCRLF(buf);
	//��������ʼ��ַ
	char* start = buf->data + buf->readPos;
	int lineSize = end - start;
	if(lineSize)
	{
		//get /xxx/xx.xx http/1.1
		//����ʽ
		char* space = memmem(start, lineSize, " ", 1);
		assert(space != NULL);
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';

		//����ľ�̬��Դ
		start = space + 1;
		space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->url[urlSize] = '\0';

		//http�汾
		start = space + 1;
		int versionSize = end - start;
		request->version = (char*)malloc(versionSize + 1);
		strncpy(request->version, start, versionSize);
		request->version[versionSize] = '\0';

		//Ϊ��������ͷ��׼��
		buf->readPos += lineSize + 2;
		//�޸�״̬
		request->curState = PaseReqHeaders;
		return true;
	}
	return false;
}


bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* buf)
{
	//����һ������
	char* end = bufferFindCRLF(buf);
	if (end != NULL)
	{
		char* start = buf->data + buf->readPos;
		int lineSize = end - start;
		//xxx: xxx
		//�����ַ��� 
		char* space = memmem(start, lineSize, ": ", 2);
		if (space != NULL)
		{
			int keySize = space - start;
			char* key = (char*)malloc(keySize + 1);
			strncpy(key, start, keySize);
			key[keySize] = '\0';

			start = space + 2;
			int valueSize = end - start;
			char* value = (char*)malloc(valueSize + 1);
			strncpy(value, start, valueSize);
			value[valueSize] = '\0';

			httpRequestAddHeader(request, key, value);
			//Ϊ��һ����׼��
			buf->data += lineSize + 2;
			return true;
		}
		else
		{
			//����ͷ��������,��������
			buf->readPos += 2;
			//�޸Ľ���״̬(�����post���޸�ΪPaseReqBody)
			request->curState = PaseReqDone;
			return true;
		}
		
	}
	return false;
}

bool parseHttpRequest(struct HttpRequest* request, struct Buffer* buf,
	struct HttpResponse* response, struct Buffer* sendBuf, int cfd)
{
	bool flag = true;
	while (request->curState != PaseReqDone && flag)
	{
		switch (request->curState)
		{
		case PaseReqLine:
			flag = parseHttpRequestLine(request, buf);
			break;
		case PaseReqHeaders:
			flag = parseHttpRequestHeader(request, buf);
			break;
		case PaseReqBody:
			break;
		default:
			break;
		}

	}
	if (request->curState == PaseReqDone)
	{
		//1.���ݽ����������ݣ���������
		processHttpRequest(request,response);
		//2.��֯��Ӧ���ݣ��������ͻ���
		httpResponsePrepareMsg(response,sendBuf,cfd);
		//3.״̬��ԭ
		request->curState = PaseReqLine;
	}
	return false;
}

bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response)
{
	//������post����
	if (!strcasecmp(request->method, "get"))
	{
		return -1;
	}
	decodeMsg(request->url, request->url);
	//����ͻ�������ľ�̬��Դ
	//1.��·����Ϊ��׼��ʽ�����·�� 
	char* file = NULL;
	//����Ǹ�Ŀ¼����Ҫ��һ��.
	if (strcmp(request->url, "/") == 0)
	{
		file = "./";
	}
	//������ǣ���ô��Ҫȥ��ǰ���"/"
	else
	{
		file = request->url + 1;  
	}
	//2.��ȡ�ļ����Է���st�У�������Ŀ¼�����ļ���
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//�ļ������� -- �ظ�404
		//��װ��Ӧ��Ϣ
		strcpy(response->fileName, "404.html");  //��װ��Ӧ��
		response->statusCode = NodFound;
		strcpy(response->statusMsg, "Not Found");
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));  //��װ��Ӧͷ
		response->sendDataFunc = sendFile;  //ָ���ظ�����
		return 0;
	}
	strcpy(response->fileName, file);  //��װ��Ӧ��
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");
	//3.�ж��ļ�����
	if (S_ISDIR(st.st_mode))  //����һ���꺯���������Ŀ¼����1�����Ƿ���0
	{
		//��Ŀ¼���ݻظ����ͻ���

		httpResponseAddHeader(response, "Content-type", getFileType(".html"));  //��װ��Ӧͷ
		response->sendDataFunc = sendDir;  //ָ���ظ�����

	}
	else
	{
		//���ļ����ݻظ����ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		char tmp[12] = { 0 };
		sprintf(tmp, "%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));  //��װ��Ӧͷ
		httpResponseAddHeader(response, "Content-length", tmp);
		response->sendDataFunc = sendFile;  //ָ���ظ�����
	}

	return false;
}

// ���ַ�ת��Ϊ������
int hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

// ����
// to �洢����֮�������, ��������, from�����������, �������
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2;
		}
		else
		{
			// �ַ�����, ��ֵ
			*to = *from;
		}

	}
	*to = '\0';
}

// a.jpg a.mp4 a.ht//�����ļ���׺�ĸ�ʽ
const char* getFileType(const char* name)
{
	// ����������ҡ�.���ַ�, �粻���ڷ���NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}

//�����ļ�����(���ļ���һ�㷢һ��)(�����׼�����·��)
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd)
{
	printf("sendfile..\n");
	//1.���ļ�
	int fd = open(fileName, O_RDONLY);
	//�����ļ���ȡ�ɹ�
	assert(fd > 0);
	//2.��������  
#if 1
	//(��һ�ַ�ʽ���ֶ�ѭ������)
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);  //���ļ�����buf����
		if (len > 0)
		{
			//send(cfd, buf, len, 0);
			bufferAppendData(sendBuf, buf, len);
#ifndef DEBUG
			bufferSendData(sendBuf, cfd);
#endif // DEBUG
			usleep(10);  //����10us����Ϊ���ļ��ܿ죬����Ҳ�ܿ죬���ǽ��մ���������÷�����һ��
		}
		//��������
		else if (len == 0)  break;
		else
		{
			close(fd);
			perror("read");
		}
	}
#else 
	//�ڶ��ַ�ʽ��ר�ź�������
	int size = lseek(fd, 0, SEEK_END);
	sendfile(cfd, fd, NULL, size);
#endif
	return 0;
}

//����Ŀ¼���� (����һ����ҳ)
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	printf("send dir..\n");
	char buf[4096] = { 0 };
	sprintf(buf, "<heml><head><title>%s</title></head><body><table>", dirName);  //�Ȱ�ͷ����ȥ
	struct dirent** namelist;
	//��ȡ��ǰĿ¼���������ļ�������
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; i++)
	{
		//ȡ���ļ�����
		char* name = namelist[i]->d_name;
		//ƴ�ӳ��Ϸ������·��
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		//��ȡ�ļ���Ϣ
		struct stat st;
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))  //�����Ŀ¼
		{
			// a��ǩ<a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else {  //������ļ�
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		//send(cfd, buf, strlen(buf), 0);
		int len = strlen(buf);
		bufferAppendData(sendBuf,buf,len);
#ifndef DEBUG
		bufferSendData(sendBuf, cfd);
#endif // DEBUG
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	//send(cfd, buf, strlen(buf), 0);
	int len = strlen(buf);
	bufferAppendData(sendBuf, buf, len);
#ifndef DEBUG
	bufferSendData(sendBuf, cfd);
#endif // DEBUG
	free(namelist);
	return 0;
}