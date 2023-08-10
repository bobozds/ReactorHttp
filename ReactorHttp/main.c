#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//通过命令行参数传入服务器的所用端口和资源目录(绝对路径)
int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    //切换服务器的工作路径(绝对路径)
    chdir(argv[2]);

    return 0;
}