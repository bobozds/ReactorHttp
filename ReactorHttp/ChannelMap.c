#include "ChannelMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ChannelMap* initChannelMap(int size)
{
	struct ChannelMap* map= (struct ChannelMap*)malloc(sizeof(struct ChannelMap));
	map->size = size;
	map->list = (struct Channel**)malloc(size * sizeof(struct Channel*));
	return map;
}

void ChannelClear(struct ChannelMap* map)
{
	if (map != NULL)  //如果mup不为空
	{
		//遍历数组
		for (int i = 0; i < map->size, i++)
		{
			if (map->list[i] != NULL)  //如果当前位置存放的也不是空
			{
				free(map->list[i]);  //释放
			}
		}
		free(map->list); //再释放掉数组
		map->list = NULL;
	}
	map->size = 0;
}

bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize)
{
	if (map->size < newSize)
	{
		int curSize = map->size;
		//容量每次增加一倍,直到超过newSize
		while (curSize < newSize)
		{
			curSize *= 2;
		}
		struct Channel** temp = realloc(map->list,curSize * unitSize);
		if (temp == NULL)
		{
			return false;
		}
		else {
			map->list = temp;
			memset(map->list[map->size], 0, (curSize - map->size)*unitSize); //需要拓展的起始地址为旧数组的最后一个+1，初始化的长度如代码
			map->size = curSize;
		}
		return true;
	}
}
