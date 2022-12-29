//
// rm_filescan.cpp
// 类RMFileScan的实现
//

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include "rm.h"
#include "rm_filescan.h"
#include "pf_pagehandle.h"
using namespace std;

RC RMFileScan::openScan(RMFilePtr& file, AttrType type,
	int len, int attroffset, Operator op, const void* val)
{
	rmfile_ = file;
	if (val != nullptr) {
		/* 做一些检查 */
		if ((len >= PF_PAGE_SIZE - sizeof(Page)) || (len <= 0)) return RM_RECSIZEMISMATCH;
		if (type == STRING0 && (len <= 0 || len > MAXSTRINGLEN))
			return RM_FCREATEFAIL;
	}
	comp_ = make_comp(type, len);
	op_ = op;
	val_ = val;
	offset_ = attroffset; /* 记录下属性的偏移量 */
	return 0;
}

//
// rewind - 从新开始遍历
// 
RC RMFileScan::rewind()
{
	curr_ = RID(1, 0);
	return 0;
}


//
// getNextRcd - 用于获取下一条记录
// 
RC RMFileScan::getNextRcd(RMRecord& rcd)
{
	uint pages = rmfile_->pagesSize();		/* 页的数目 */
	uint slots = 0;		/* slots的数目 */

	for (uint i = curr_.page(); i <= pages; i++) {
		PFPageHandle page = PFGetPage(rmfile_->pffile_, i);
		Ptr addr = page.rawPtr();
		//RMPageHdr hdr(slots, addr);
		RMPageHdrEx* hdr = NULL;
		hdr = (RMPageHdrEx*)addr;
		uint slot = curr_.page() == i ? curr_.slot() + 1 : 1;

		for (; slot <= hdr->sum; slot++) {
			curr_ = RID(i, slot);
			if (rmfile_.get()->isValidRID_sc(curr_)) {		/* 这个slot已经被使用了 */
				//char* addr;
				int len;
				char* addr0;
				rmfile_->getRcd(curr_.page(),curr_.slot(),addr0,len);	/* 获取一条记录 */
				//rmfile_->getRcd(1, 4, addr0, len);
				//printf("%d,%d len: %d\n", 1, 4, len);
				rcd.set(addr0, len, curr_);
				addr = rcd.rawPtr();
				//printf("%s,%s\n", addr + offset_, val_);
				if (comp_->eval(addr + offset_, op_, val_)) 
				{
					//rmfile_->getRcd(1, 4, addr0, len);
					//printf("%d,%d len: %d\n", 1, 4, len);
					return 0;
				}
				/* 否则的话,获取下一条记录 */
			}
			//if()

		}
	}
	return RM_EOF;
}

RC RMFileScan::closeScan()
{
	if (comp_ != nullptr) delete comp_;
	curr_ = RID(1, 0);
	return 0;
}