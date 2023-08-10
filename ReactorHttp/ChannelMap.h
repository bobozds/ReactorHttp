#pragma once
#include <stdbool.h>
struct ChannelMap
{
	int size; //数组的长度
	struct Channel** list; //一个存放Channel*的数组,并且跟fd存在映射关系（下标为几，fd就放在几）
};

//初始化
struct ChannelMap* initChannelMap(int size);
//清空Map
void ChannelClear(struct ChannelMap* map);
//重新分配空间
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);