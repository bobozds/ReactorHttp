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
	//把键值对释放掉
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
		//把键值对数组释放掉
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
	//请求行结束地址
	char* end = bufferFindCRLF(buf);
	//请求行起始地址
	char* start = buf->data + buf->readPos;
	int lineSize = end - start;
	if(lineSize)
	{
		//get /xxx/xx.xx http/1.1
		//请求方式
		char* space = memmem(start, lineSize, " ", 1);
		assert(space != NULL);
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';

		//请求的静态资源
		start = space + 1;
		space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->url[urlSize] = '\0';

		//http版本
		start = space + 1;
		int versionSize = end - start;
		request->version = (char*)malloc(versionSize + 1);
		strncpy(request->version, start, versionSize);
		request->version[versionSize] = '\0';

		//为解析请求头做准备
		buf->readPos += lineSize + 2;
		//修改状态
		request->curState = PaseReqHeaders;
		return true;
	}
	return false;
}


bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* buf)
{
	//读出一行数据
	char* end = bufferFindCRLF(buf);
	if (end != NULL)
	{
		char* start = buf->data + buf->readPos;
		int lineSize = end - start;
		//xxx: xxx
		//搜索字符串 
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
			//为下一行做准备
			buf->data += lineSize + 2;
			return true;
		}
		else
		{
			//请求头解析完了,跳过空行
			buf->readPos += 2;
			//修改解析状态(如果有post，修改为PaseReqBody)
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
		//1.根据解析出的数据，做出处理
		processHttpRequest(request,response);
		//2.组织响应数据，并发给客户端
		httpResponsePrepareMsg(response,sendBuf,cfd);
		//3.状态还原
		request->curState = PaseReqLine;
	}
	return false;
}

bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response)
{
	//不解析post请求
	if (!strcasecmp(request->method, "get"))
	{
		return -1;
	}
	decodeMsg(request->url, request->url);
	//处理客户端请求的静态资源
	//1.将路径换为标准格式的相对路径 
	char* file = NULL;
	//如果是根目录，就要加一个.
	if (strcmp(request->url, "/") == 0)
	{
		file = "./";
	}
	//如果不是，那么就要去掉前面的"/"
	else
	{
		file = request->url + 1;  
	}
	//2.获取文件属性放入st中（可能是目录或者文件）
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		//文件不存在 -- 回复404
		//组装响应消息
		strcpy(response->fileName, "404.html");  //组装响应行
		response->statusCode = NodFound;
		strcpy(response->statusMsg, "Not Found");
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));  //组装响应头
		response->sendDataFunc = sendFile;  //指定回复函数
		return 0;
	}
	strcpy(response->fileName, file);  //组装响应行
	response->statusCode = OK;
	strcpy(response->statusMsg, "OK");
	//3.判断文件类型
	if (S_ISDIR(st.st_mode))  //这是一个宏函数，如果是目录返回1，不是返回0
	{
		//将目录数据回复给客户端

		httpResponseAddHeader(response, "Content-type", getFileType(".html"));  //组装响应头
		response->sendDataFunc = sendDir;  //指定回复函数

	}
	else
	{
		//将文件数据回复给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		char tmp[12] = { 0 };
		sprintf(tmp, "%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));  //组装响应头
		httpResponseAddHeader(response, "Content-length", tmp);
		response->sendDataFunc = sendFile;  //指定回复函数
	}

	return false;
}

// 将字符转换为整形数
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

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
	for (; *from != '\0'; ++to, ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			*to = *from;
		}

	}
	*to = '\0';
}

// a.jpg a.mp4 a.ht//解析文件后缀的格式
const char* getFileType(const char* name)
{
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
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

//发送文件函数(将文件读一点发一点)(传入标准的相对路径)
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd)
{
	printf("sendfile..\n");
	//1.打开文件
	int fd = open(fileName, O_RDONLY);
	//断言文件读取成功
	assert(fd > 0);
	//2.发送数据  
#if 1
	//(第一种方式，手动循环发送)
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);  //将文件读在buf里面
		if (len > 0)
		{
			//send(cfd, buf, len, 0);
			bufferAppendData(sendBuf, buf, len);
#ifndef DEBUG
			bufferSendData(sendBuf, cfd);
#endif // DEBUG
			usleep(10);  //休眠10us，因为读文件很快，发送也很快，但是接收处理很慢，让发送慢一点
		}
		//发送完了
		else if (len == 0)  break;
		else
		{
			close(fd);
			perror("read");
		}
	}
#else 
	//第二种方式，专门函数发送
	int size = lseek(fd, 0, SEEK_END);
	sendfile(cfd, fd, NULL, size);
#endif
	return 0;
}

//发送目录函数 (发送一个网页)
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
	printf("send dir..\n");
	char buf[4096] = { 0 };
	sprintf(buf, "<heml><head><title>%s</title></head><body><table>", dirName);  //先把头发过去
	struct dirent** namelist;
	//获取当前目录下面所有文件的名字
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; i++)
	{
		//取出文件名字
		char* name = namelist[i]->d_name;
		//拼接出合法的相对路径
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		//获取文件信息
		struct stat st;
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))  //如果是目录
		{
			// a标签<a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else {  //如果是文件
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