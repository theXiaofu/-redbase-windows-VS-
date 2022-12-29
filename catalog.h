#ifndef CATALOG_H
#define CATALOG_H
#include <string.h>
//#include "printer.h"
#include "redbase.h"

#pragma pack(1)
struct DataRel
{
	int rcdlen;			// ��¼�ĳ���
	int attrcount;		// ���Ե�����
	int pages;			// ʹ�õ�ҳ����Ŀ
	int rcds;
	char relname[MAXNAME + 1];

	DataRel()
	{
		memset(relname, 0, MAXNAME + 1);
	}

	static int members() { return 5; }
	//
	// spaceUsage - ռ�ÿռ��С,���洢���ļ���Ӧ��ռ�ö��ٿռ�,��Ҫע�����,���ļ���
	// һ���ýṹ�ļ�¼�ǽ��մ洢��,����Ϊ�˶�������ֿն�.
	// 
	static int spaceUsage()
	{
		return sizeof(int) * 4 + MAXNAME + 1;
	}
};
#pragma pack()
#endif /* CATALOG_H */