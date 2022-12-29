#ifndef RM_FILEHANDLE_H
#define RM_FILEHANDLE_H
#include "redbase.h"
#include "noncopyable.h"
#include "rm.h"
#include "pf.h"
#include "rm_rid.h"
//#include "rm_pagehdr.h"
//#include "rm_record.h"
#include "pf_filehandle.h"
#include "pf_pagehandle.h"
#include "rm_record.h"
class PFFileHandle;

class RMFileHandle : public noncopyable 
{
	friend class RMFileHandleTest;
	friend class RMFileScan;
	friend class RMManager;
public:
	RMFileHandle(PFFilePtr file);
	~RMFileHandle();
public:
	RC getRcd(Page page_num,int num, char*&addr_,int& length);
	uint pagesSize() const
	{
		return rmhdr_.size;
	}
	RC insertRcd(const Ptr addr,int len,int& page_num, int& num);
	RC deleteRcd(const RID& rid);
	RC updateRcd(const RMRecord& rcd);
	RC forcePages(Page num = ALL_PAGES);
	bool isValidRID_sc(RID rid);
	
private:
	bool isValidPage(Page num) const;
	bool isValidRID(const RID& rid) const;
	
	//bool nextFreeSlot(PFPageHandle& page, Page& num, Slot& slot);
	int getPages() const { return rmhdr_.size; }
public:
	//uint capacity_;		// ÿ��ҳ��֧�ֵĵ�slot����Ŀ
	PFFilePtr pffile_;
	RMFileHdr rmhdr_;
	//int rcdlen_;		// ÿһ����¼�ĳ���
	bool changed_;		// �ļ�ͷ��Ϣ�Ƿ��Ѿ��ı�
};

using RMFilePtr = shared_ptr<RMFileHandle>;

#endif /* RM_FILEHANDLE */