#pragma once
#include <stdbool.h>
struct ChannelMap
{
	int size; //����ĳ���
	struct Channel** list; //һ�����Channel*������,���Ҹ�fd����ӳ���ϵ���±�Ϊ����fd�ͷ��ڼ���
};

//��ʼ��
struct ChannelMap* initChannelMap(int size);
//���Map
void ChannelClear(struct ChannelMap* map);
//���·���ռ�
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize);