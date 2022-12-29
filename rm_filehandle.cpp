#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include "rm.h"
#include "rm_error.h"
#include "rm_filehandle.h"
//#include "rm_pagehdr.h"
using namespace std;


//
// CalcSlotsSize - 计算一个数据块最多所支持的存储的记录的条数
// 
//uint static CalcSlotsCapacity(uint len)
//{
//	uint remain = PF_PAGE_SIZE - sizeof(RMPageHdrEx); // 剩余可用的字节数目
//	// floor - 向下取整
//	// 每一条记录都需要1个bit,也就是1/8字节来表示是否已经记录了数据
//	uint slots = floor((1.0 * remain) / (len + 1 / 8));
//	uint hdr_size = sizeof(RMPageHdrEx) + BitMap::bytes(slots);
//	// 接下来需要不断调整
//	while ((slots * len + hdr_size) > PF_PAGE_SIZE) {
//		slots--;
//		hdr_size = sizeof(RMPageHdrEx) + BitMap::bytes(slots);
//	}
//	return slots;
//}


RMFileHandle::RMFileHandle(PFFilePtr file)
	: changed_(false)
{
	pffile_ = file;
	PFPageHandle page = PFGetPage(file, 0);
	Ptr buff = page.rawPtr();
	memcpy(&rmhdr_, buff, sizeof(RMFileHdr));
	//printf("rmhdr_:%d", rmhdr_.free);
	//rcdlen_ = rmhdr_.rcdlen;
	
	//capacity_ = CalcSlotsCapacity(rcdlen_);
}

RMFileHandle::~RMFileHandle()
{
	if (changed_) 
	{
		PFPageHandle page = PFGetPage(pffile_, 0);
		Ptr buff = page.rawPtr();
		memcpy(buff, &rmhdr_, sizeof(RMFileHdr));
		//printf("set_dirty");
		//printf("%d\n", rmhdr_.free);
		page.setDirty();
	}
}


//
// isValidPage - 判断是否为有效的页面
// 
bool RMFileHandle::isValidPage(Page num) const
{
	return (num >= 0) && (num <= rmhdr_.size);
}

bool RMFileHandle::isValidRID(const RID& rid) const
{
	Slot slot = rid.slot();
	return isValidPage(rid.page()) && slot >= 0;
}

bool RMFileHandle::isValidRID_sc(RID rid)
{
	bool valid = false;
	if (!isValidRID(rid)) return false;
	Page num = rid.page();
	Slot pos = rid.slot();
	PFPageHandle page = PFGetPage(pffile_, num);	// 获取对应的页面
	Ptr addr = page.rawPtr();

	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;

	if (hdr->sum != 0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
		// 强制类型转换成int类型，方便取值
		int* pos = (int*)curr;
		if (*pos == -1)
			valid = false;
		else
			valid = true;
	}
	else
	{
		return END_RM_ERR;
	}

	//--hdr->sum;
	//page.setDirty();
	return valid;
}

//
// insertRcd - 插入一条记录
// 
RC RMFileHandle::insertRcd(const Ptr recd, int len,int &page_num,int &  rec_num)
{
	RC rc;
	PFPageHandle page;
	Page current_num;
	RMPageHdrEx* hdr = NULL;
	Ptr addr;
	current_num = rmhdr_.free;
	//printf("currentnum:%d\n", current_num);
	if (PF_PAGE_SIZE - sizeof(RMPageHdrEx)< len + 2*sizeof(int))//记录长度大于页的大小去除页头的长度
		return RM_SIZETOOBIG;
	if (current_num != PAGE_LIST_END)
	{
		page = PFGetPage(pffile_,current_num);
		addr = page.rawPtr();
		hdr = (RMPageHdrEx*)addr;
	}
	if (current_num == PAGE_LIST_END|| hdr->full
		|| hdr->remain < (len + sizeof(int) * 2))
	{

		//printf("intt0:\n");
		if (current_num != PAGE_LIST_END)//满足条件需要取消固定
		{
			//pffile_.get()->unpin(current_num);
			//printf("intt01:\n");
		}
		/*if (current_num == PAGE_LIST_END)
			printf("intt01:\n");
		else
		{
			if (hdr->full)
				printf("intt02:\n");
			if (hdr->remain < (len + sizeof(int) * 2))
				printf("intt03:\n");
		}*/
		//说明当前的空闲页链为空需要申请新的页
		page = PFAllocPage(pffile_);
		current_num = page.page();
		rmhdr_.free = current_num;//将新申请的页加到空闲页链上
		
		rmhdr_.size++;
		//printf("size:%d\n", rmhdr_.size);
		changed_ = true;
		addr = page.rawPtr();

		// 页的第一块是头部信息
		hdr = (RMPageHdrEx*)addr;
		hdr->free = PAGE_LIST_END;//初始化为链尾
		hdr->full = false;		  // 初始化为空
		hdr->remain = PF_PAGE_SIZE - sizeof(RMPageHdrEx);
		hdr->sum = 0;
		//page.setDirty();
		//pfManager.closeFile(file);
	}
	int recAddr = 0;
	if(hdr->sum!=0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (hdr->sum - 1) + sizeof(RMPageHdrEx)]);
		// 强制类型转换成int类型，方便取值
		int* beginAddr = (int*)curr;
		// 将currAddr计算出来
		recAddr = *beginAddr - len;
		// 将地址指针转移到下一个标识应填写的位置
		curr = curr + sizeof(int) * 2;
		int* pos = (int*)curr;
		//将即将存入的数据的头地址写入pos
		*pos = recAddr;
		curr += sizeof(int);
		pos = (int*)curr;
		//将当前即将存入的数据的长度写入pos+sizeof(int)
		*pos = len;
	}
	else
	{
		char* curr = &(addr[sizeof(RMPageHdrEx)]);
		int* pos = (int*)curr;
		recAddr = PF_PAGE_SIZE - len;
		*pos = recAddr;
		curr += sizeof(int);
		pos = (int*)curr;
		*pos = len;
	}
	page_num = current_num;
	memcpy(&addr[recAddr], recd, len);
	//if (hdr->sum < 5)
	//{
	//	for (int i = sizeof(RMPageHdrEx); i < PF_PAGE_SIZE; i++)
	//	{
	//		cout << short(addr[i]) << " ";
	//	}
	//	cout << endl;
	//}
	//strcpy(&addr[recAddr], recd);
	(hdr->sum)++;
	rec_num = hdr->sum;
	hdr->remain -= (sizeof(int) * 2 + len);
	//pffile_.get().unpin(current_num);
	//pffile_.get()->unpin(current_num);
	if ((hdr->remain / 4092.0) < 0.2)
	{
		hdr->full = true;
	}
	page.setDirty();

	return 0;
}

//
//  updateRcd 更新一条记录, rcd中记录了记录在磁盘中的位置
// 
//RC RMFileHandle::updateRcd(const RMRecord& rcd)
//{
//	RID rid;
//	rcd.getRid(rid);
//	if (!isValidRID(rid)) return RM_BAD_RID;
//	Page num = rid.page();
//	Slot pos = rid.slot();
//
//	Ptr src;
//	PFPageHandle page = PFGetPage(pffile_, num);
//	Ptr dst = page.rawPtr();
//
//	RMPageHdr hdr(capacity_, dst);
//	if (hdr.map.available(pos)) return RM_NORECATRID;
//	rcd.getData(src);
//	dst = dst + hdr.lenOfHdr() + pos * rcdlen_;
//	memcpy(dst, src, rcdlen_);
//	page.setDirty();
//	return 0;
//}

RC RMFileHandle::deleteRcd(const RID& rid)
{
	if (!isValidRID(rid)) return RM_BAD_RID;
	Page num = rid.page();
	Slot pos = rid.slot();
	PFPageHandle page = PFGetPage(pffile_, num);	// 获取对应的页面
	Ptr addr = page.rawPtr();
	
	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;

	if (hdr->sum != 0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
		// 强制类型转换成int类型，方便取值
		int* pos = (int*)curr;
		*pos = -1;
		//将长度设为 0
		curr += sizeof(int);
		pos = (int*)curr;
		*pos = 0;
	}
	else
	{
		return END_RM_ERR;
	}

	//--hdr->sum;
	//page.setDirty();
	RC rc2;
	if ((rc2 = pffile_.get()->markDirty(num)) || (rc2 = pffile_.get()->unpin(num)))//因为涉及到更改记录需要反馈到硬盘，需要mark
		return (rc2);
	return 0;
}

RC RMFileHandle::updateRcd(const RMRecord& rcd) {
	RID rid;
	rcd.getRid(rid);
	if (!isValidRID(rid)) return RM_BAD_RID;
	Page num = rid.page();
	Slot pos = rid.slot();
	Ptr src;
	PFPageHandle page = PFGetPage(pffile_, num);	// 获取对应的页面
	Ptr addr = page.rawPtr();

	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;
	char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
	// 强制类型转换成int类型，方便取值
	int* beginAddr = (int*)curr;
	curr = curr + sizeof(int);
	int* len = (int *)curr;
	rcd.getData(src);
	memcpy(&addr[*beginAddr], src, *len);
	page.setDirty();
	return 0;
}

 /*getRcd - 获取一条记录*/
 
RC RMFileHandle::getRcd(Page page_num,int num ,char*&addr_,int& length)
{
	RC rc;
	PFPageHandle page;
	RMPageHdrEx* hdr = NULL;
	Ptr addr;
	//通过页号获取页
	page = PFGetPage(pffile_, page_num);
	addr = page.rawPtr();
	hdr = (RMPageHdrEx*)addr;
	addr = page.rawPtr();
	//if (page_num == 1 && num == 4)
		//system("pause");
	// 页的第一块是头部信息
	hdr = (RMPageHdrEx*)addr;
	char* curr = &(addr[sizeof(int) * 2 * (num - 1) + sizeof(RMPageHdrEx)]);
	// 确定记录的地址
	int* beginAddr = (int*)curr;
	// 将地址指针转移到记录长度
	curr = curr + sizeof(int);
	//确定记录的长度
	int* len = (int*)curr;
	//获取记录
	addr_ = &addr[*beginAddr];
	//获取记录长度
	length = *len;					
	
	//if (!isValidRID(rid)) return RM_BAD_RID;
	//Page num = rid.page();
	//Slot pos = rid.slot();
	//PFPageHandle page = PFGetPage(pffile_, num);
	//Ptr addr = page.rawPtr();
	//RMPageHdr hdr(capacity_, addr);
	//if (hdr.map.available(pos)) return RM_NORECATRID; // 该pos必定不会空闲
	//addr = addr + hdr.lenOfHdr() + pos * rcdlen_;
	//rcd.set(addr, rmhdr_.rcdlen, rid);		// 设定记录
	//return 0;
	return 0;
}

RC RMFileHandle::forcePages(Page num /* = ALL_PAGES */)
{
	if (!isValidPage(num) && num != ALL_PAGES)
		return RM_BAD_RID;
	return pffile_->forcePages(num);
}

//
// getNextFreeSlot - 获取下一个空闲的pos,一个页面包含很多pos
// 
//bool RMFileHandle::nextFreeSlot(PFPageHandle& page, Page& num, Slot& slot)
//{
//	Ptr addr;
//	if (rmhdr_.free > 0) { // 仍然有空闲的页面
//		page = PFGetPage(pffile_, rmhdr_.free);
//		num = rmhdr_.free;
//		addr = page.rawPtr();
//	}
//	else { // 需要重新分配页面
//		page = PFAllocPage(pffile_);
//		addr = page.rawPtr();
//		num = page.page();
//		RMPageHdr hdr(capacity_, addr);
//		hdr.setNext(PAGE_LIST_END);
//		hdr.setRemain(capacity_);
//		int remain = hdr.remain();
//		hdr.map.setAll();
//		page.setDirty();
//		rmhdr_.free = num; // 将分配的页面的页号添加到空闲链表中
//		rmhdr_.size++;
//		changed_ = true;
//	}

//	RMPageHdr hdr(capacity_, addr);
//	for (int i = 0; i < capacity_; i++) {
//		if (hdr.map.available(i)) { // 该位置恰好可用
//			slot = i;
//			return true;
//		}
//	}
//	return false; // unexpected error
//}




