
#ifndef RM_RID_H
#define RM_RID_H

#include "redbase.h"
#include "pf.h"
#include <iostream>
#include <assert.h>
#include <string.h>
#include "rm_error.h"
using namespace std;

using Slot = int;

//
// RID - Record id interface
// 
struct RID 
{
	Page num;
	Slot st;
	RID() : num(-1), st(-1) {}
	RID(Page num, Slot st) : num(num), st(st) {}
	~RID() {};
	Page page() const { return num; }
	Slot slot() const { return st; }
	bool operator==(const RID& rhs) const
	{
		return (num == rhs.num) && (st == rhs.st);
	}
	RID& operator=(const RID& rhs)
	{
		if (this == &rhs) return *this;
		num = rhs.num;
		st = rhs.st;
		return *this;
	}
	/*
 *返回RID的页码
 */
	RC GetPageNum(Page& pageNum) const {
		//if(page == INVALID_PAGE) return RM_INVALIDRID;
		pageNum = num;
		return 0;
	}

	/*
	 * 返回RID的slot
	 */
	RC GetSlotNum(Slot& slotNum) const {
		//if(slot == INVALID_SLOT) return RM_INVALIDRID;
		slotNum = st;
		return 0;
	}
	RC set(int p, int s)
	{
		num = p; st = s;
		return 0;
	}
	/*
	 * 检验RID是否可用
	 */
	RC isValidRID() const {
		if (num > 0 && st >= 0)
			return 0;
		else
			return RM_INVALIDRID;
	}
};

ostream& operator <<(ostream& os, const RID& r);
#endif /* RM_RID_H */