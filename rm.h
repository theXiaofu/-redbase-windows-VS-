// 
//	Record Manager component interface
//	

#ifndef RM_H
#define RM_H

#include "comp.h"
typedef unsigned int uint;
//
// RMFileHdr - �ļ�ͷ��
// 
#pragma pack(1)
struct RMFileHdr {
	int free;				// �����е�һ������ҳ
	int size;				// �ļ����Ѿ������˵�ҳ����Ŀ
	//uint rcdlen;			// ��¼�Ĵ�С
} ;
#pragma pack()
#pragma pack(1)
struct RMPageHdrEx 
{
	int free;//ָ����һ������ҳ
	//int next;//���ָ����һ������ҳ
	int full;//�Ƿ�װ��
	int remain;//ʣ��ռ�
	int sum;//ҳ�м�¼�ĸ���
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