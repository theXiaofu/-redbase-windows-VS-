//
// rm_filescan.cpp
// ��RMFileScan��ʵ��
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
		/* ��һЩ��� */
		if ((len >= PF_PAGE_SIZE - sizeof(Page)) || (len <= 0)) return RM_RECSIZEMISMATCH;
		if (type == STRING0 && (len <= 0 || len > MAXSTRINGLEN))
			return RM_FCREATEFAIL;
	}
	comp_ = make_comp(type, len);
	op_ = op;
	val_ = val;
	offset_ = attroffset; /* ��¼�����Ե�ƫ���� */
	return 0;
}

//
// rewind - ���¿�ʼ����
// 
RC RMFileScan::rewind()
{
	curr_ = RID(1, 0);
	return 0;
}


//
// getNextRcd - ���ڻ�ȡ��һ����¼
// 
RC RMFileScan::getNextRcd(RMRecord& rcd)
{
	uint pages = rmfile_->pagesSize();		/* ҳ����Ŀ */
	uint slots = 0;		/* slots����Ŀ */

	for (uint i = curr_.page(); i <= pages; i++) {
		PFPageHandle page = PFGetPage(rmfile_->pffile_, i);
		Ptr addr = page.rawPtr();
		//RMPageHdr hdr(slots, addr);
		RMPageHdrEx* hdr = NULL;
		hdr = (RMPageHdrEx*)addr;
		uint slot = curr_.page() == i ? curr_.slot() + 1 : 1;

		for (; slot <= hdr->sum; slot++) {
			curr_ = RID(i, slot);
			if (rmfile_.get()->isValidRID_sc(curr_)) {		/* ���slot�Ѿ���ʹ���� */
				//char* addr;
				int len;
				char* addr0;
				rmfile_->getRcd(curr_.page(),curr_.slot(),addr0,len);	/* ��ȡһ����¼ */
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
				/* ����Ļ�,��ȡ��һ����¼ */
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