//
// rm_record.cpp
// ��RMRecord��ʵ��.
//
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "rm.h"
#include "rm_rid.h"
#include "rm_record.h"

using namespace std;


RMRecord::RMRecord()
	:rcdlen_(-1), addr_(nullptr), rid_(-1, -1)
{}

RMRecord::~RMRecord()
{
	if (addr_ != nullptr) {
		free(addr_);
	}
}

//
// set - �趨��¼����Ϣ,��src��������һ����¼,����¼�¼�¼�Ĵ�С,λ����Ϣ
// 
RC RMRecord::set(Ptr src, uint len, RID rid)
{
	if ((rcdlen_ != -1) && (len != rcdlen_)) { free(addr_);};
	rcdlen_ = len;		 // ��¼��ÿһ����¼�Ĵ�С
	rid_ = rid;		 // ��¼�¶�Ӧ��ϵ
	if (addr_ == nullptr) addr_ = (char*)malloc(sizeof(char) * len);
	memcpy(addr_, src, len);
	return 0;
	//strcpy(addr_, addr_);
}


RC RMRecord::getData(Ptr& p) const
{
	if (addr_ != nullptr && rcdlen_ != -1) {
		p = addr_;
		return 0;
	}
	else return RM_NULLRECORD;
}

RC RMRecord::getRid(RID& rid) const
{
	if (addr_ != nullptr && rcdlen_ != -1)
	{
		rid = rid_;
		return 0;
	}
	else return RM_NULLRECORD;
}

ostream& operator<<(ostream& os, const RID& r)
{
	os << "[" << r.page() << "," << r.slot() << "]";
	return os;
}

