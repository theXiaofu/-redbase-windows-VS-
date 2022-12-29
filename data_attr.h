#ifndef DATA_ATTR_H
#define DATA_ATTR_H
#include <string.h>
#include "redbase.h"

//
// DataAttr - ��Ҫ�����������е�����
//
#pragma pack(1)
struct DataAttr {
	int offset;						// ƫ����
	AttrType type;					// ����
	int len;						// ����
	int idxno;						// ������������Ļ�,�����ı��
	char relname[MAXNAME + 1];		// 
	char attrname[MAXNAME + 1];		// ���Ե�����
public:
	static int members() { return 7; }
	static int spaceUsage()
	{
		return sizeof(int) * 3 + sizeof(AttrType) + 2 * (MAXNAME + 1) + sizeof(AggFun);
	}
	DataAttr() : offset(-1)
	{
		idxno = -1;
		memset(relname, 0, MAXNAME + 1);
		memset(attrname, 0, MAXNAME + 1);
	}

	DataAttr(const DataAttr& rhs)
		: type(rhs.type), len(rhs.len)
		, offset(rhs.offset), idxno(rhs.idxno)
	{
		// tofix
		memcpy(attrname, rhs.attrname, MAXNAME + 1);
		memcpy(relname, rhs.relname, MAXNAME + 1);
	}

	DataAttr& operator=(const DataAttr& rhs)
	{
		if (this != &rhs) {
			strcpy(relname, rhs.relname);
			strcpy(attrname, rhs.attrname);
			offset = rhs.offset;
			idxno = rhs.idxno;
			len = rhs.len;
			type = rhs.type;
		}
		return *this;
	}
};
#pragma pack()


#endif /* DATA_ATTR_H */