// 
//	Record Manager component interface
//	

#ifndef RM_H
#define RM_H

#include "comp.h"
typedef unsigned int uint;
//
// RMFileHdr - 文件头部
// 
#pragma pack(1)
struct RMFileHdr {
	int free;				// 链表中第一个空闲页
	int size;				// 文件中已经分配了的页的数目
	//uint rcdlen;			// 记录的大小
} ;
#pragma pack()
#pragma pack(1)
struct RMPageHdrEx 
{
	int free;//指向下一个空闲页
	//int next;//如果指向下一个数据页
	int full;//是否装满
	int remain;//剩余空间
	int sum;//页中记录的个数
	//char slotMap[];
	//bool isavailable(Slot i);
};
#pragma pack()
/*

		|-----------------------|
		|	free				|
		|-----------------------|
		|	free				|
		|-----------------------|
		|	slots				|
		|-----------------------|
		|	remain				|
		|-----------------------|
		|	slotMap				|
		|						|
		|						|
		|						|
		|-----------------------|

 */

#endif /* RM_H */