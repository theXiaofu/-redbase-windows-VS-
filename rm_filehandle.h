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
	//uint capacity_;		// 每个页可支持的的slot的数目
	PFFilePtr pffile_;
	RMFileHdr rmhdr_;
	//int rcdlen_;		// 每一条记录的长度
	bool changed_;		// 文件头信息是否已经改变
};

using RMFilePtr = shared_ptr<RMFileHandle>;

#endif /* RM_FILEHANDLE */