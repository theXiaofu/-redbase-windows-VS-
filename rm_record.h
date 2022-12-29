#ifndef RM_RECORD_H
#define RM_RECORD_H
#include "rm_error.h"
//#include "redbase.h"
#include "rm_rid.h"
//
// RMRecord
// 
class RMRecord {
	friend class RMRecordTest;
public:
	RMRecord();
	~RMRecord();
public:
	RC getData(Ptr& p) const;
	RC set(Ptr p, uint size, RID rid);
	RC getRid(RID& rid) const;
	Ptr rawPtr() { return addr_; }
	RID& rid() { return rid_; }
private:
	uint rcdlen_;	// ��¼��ռ���ֽڴ�С
	Ptr addr_;		// �ֽڵĵ�ַ
	RID rid_;		// ��¼���ļ��е�λ��
};

#endif