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
// CalcSlotsSize - ����һ�����ݿ������֧�ֵĴ洢�ļ�¼������
// 
//uint static CalcSlotsCapacity(uint len)
//{
//	uint remain = PF_PAGE_SIZE - sizeof(RMPageHdrEx); // ʣ����õ��ֽ���Ŀ
//	// floor - ����ȡ��
//	// ÿһ����¼����Ҫ1��bit,Ҳ����1/8�ֽ�����ʾ�Ƿ��Ѿ���¼������
//	uint slots = floor((1.0 * remain) / (len + 1 / 8));
//	uint hdr_size = sizeof(RMPageHdrEx) + BitMap::bytes(slots);
//	// ��������Ҫ���ϵ���
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
// isValidPage - �ж��Ƿ�Ϊ��Ч��ҳ��
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
	PFPageHandle page = PFGetPage(pffile_, num);	// ��ȡ��Ӧ��ҳ��
	Ptr addr = page.rawPtr();

	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;

	if (hdr->sum != 0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
		// ǿ������ת����int���ͣ�����ȡֵ
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
// insertRcd - ����һ����¼
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
	if (PF_PAGE_SIZE - sizeof(RMPageHdrEx)< len + 2*sizeof(int))//��¼���ȴ���ҳ�Ĵ�Сȥ��ҳͷ�ĳ���
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
		if (current_num != PAGE_LIST_END)//����������Ҫȡ���̶�
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
		//˵����ǰ�Ŀ���ҳ��Ϊ����Ҫ�����µ�ҳ
		page = PFAllocPage(pffile_);
		current_num = page.page();
		rmhdr_.free = current_num;//���������ҳ�ӵ�����ҳ����
		
		rmhdr_.size++;
		//printf("size:%d\n", rmhdr_.size);
		changed_ = true;
		addr = page.rawPtr();

		// ҳ�ĵ�һ����ͷ����Ϣ
		hdr = (RMPageHdrEx*)addr;
		hdr->free = PAGE_LIST_END;//��ʼ��Ϊ��β
		hdr->full = false;		  // ��ʼ��Ϊ��
		hdr->remain = PF_PAGE_SIZE - sizeof(RMPageHdrEx);
		hdr->sum = 0;
		//page.setDirty();
		//pfManager.closeFile(file);
	}
	int recAddr = 0;
	if(hdr->sum!=0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (hdr->sum - 1) + sizeof(RMPageHdrEx)]);
		// ǿ������ת����int���ͣ�����ȡֵ
		int* beginAddr = (int*)curr;
		// ��currAddr�������
		recAddr = *beginAddr - len;
		// ����ַָ��ת�Ƶ���һ����ʶӦ��д��λ��
		curr = curr + sizeof(int) * 2;
		int* pos = (int*)curr;
		//��������������ݵ�ͷ��ַд��pos
		*pos = recAddr;
		curr += sizeof(int);
		pos = (int*)curr;
		//����ǰ������������ݵĳ���д��pos+sizeof(int)
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
//  updateRcd ����һ����¼, rcd�м�¼�˼�¼�ڴ����е�λ��
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
	PFPageHandle page = PFGetPage(pffile_, num);	// ��ȡ��Ӧ��ҳ��
	Ptr addr = page.rawPtr();
	
	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;

	if (hdr->sum != 0)
	{
		char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
		// ǿ������ת����int���ͣ�����ȡֵ
		int* pos = (int*)curr;
		*pos = -1;
		//��������Ϊ 0
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
	if ((rc2 = pffile_.get()->markDirty(num)) || (rc2 = pffile_.get()->unpin(num)))//��Ϊ�漰�����ļ�¼��Ҫ������Ӳ�̣���Ҫmark
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
	PFPageHandle page = PFGetPage(pffile_, num);	// ��ȡ��Ӧ��ҳ��
	Ptr addr = page.rawPtr();

	RMPageHdrEx* hdr = (RMPageHdrEx*)addr;
	char* curr = &(addr[sizeof(int) * 2 * (pos - 1) + sizeof(RMPageHdrEx)]);
	// ǿ������ת����int���ͣ�����ȡֵ
	int* beginAddr = (int*)curr;
	curr = curr + sizeof(int);
	int* len = (int *)curr;
	rcd.getData(src);
	memcpy(&addr[*beginAddr], src, *len);
	page.setDirty();
	return 0;
}

 /*getRcd - ��ȡһ����¼*/
 
RC RMFileHandle::getRcd(Page page_num,int num ,char*&addr_,int& length)
{
	RC rc;
	PFPageHandle page;
	RMPageHdrEx* hdr = NULL;
	Ptr addr;
	//ͨ��ҳ�Ż�ȡҳ
	page = PFGetPage(pffile_, page_num);
	addr = page.rawPtr();
	hdr = (RMPageHdrEx*)addr;
	addr = page.rawPtr();
	//if (page_num == 1 && num == 4)
		//system("pause");
	// ҳ�ĵ�һ����ͷ����Ϣ
	hdr = (RMPageHdrEx*)addr;
	char* curr = &(addr[sizeof(int) * 2 * (num - 1) + sizeof(RMPageHdrEx)]);
	// ȷ����¼�ĵ�ַ
	int* beginAddr = (int*)curr;
	// ����ַָ��ת�Ƶ���¼����
	curr = curr + sizeof(int);
	//ȷ����¼�ĳ���
	int* len = (int*)curr;
	//��ȡ��¼
	addr_ = &addr[*beginAddr];
	//��ȡ��¼����
	length = *len;					
	
	//if (!isValidRID(rid)) return RM_BAD_RID;
	//Page num = rid.page();
	//Slot pos = rid.slot();
	//PFPageHandle page = PFGetPage(pffile_, num);
	//Ptr addr = page.rawPtr();
	//RMPageHdr hdr(capacity_, addr);
	//if (hdr.map.available(pos)) return RM_NORECATRID; // ��pos�ض��������
	//addr = addr + hdr.lenOfHdr() + pos * rcdlen_;
	//rcd.set(addr, rmhdr_.rcdlen, rid);		// �趨��¼
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
// getNextFreeSlot - ��ȡ��һ�����е�pos,һ��ҳ������ܶ�pos
// 
//bool RMFileHandle::nextFreeSlot(PFPageHandle& page, Page& num, Slot& slot)
//{
//	Ptr addr;
//	if (rmhdr_.free > 0) { // ��Ȼ�п��е�ҳ��
//		page = PFGetPage(pffile_, rmhdr_.free);
//		num = rmhdr_.free;
//		addr = page.rawPtr();
//	}
//	else { // ��Ҫ���·���ҳ��
//		page = PFAllocPage(pffile_);
//		addr = page.rawPtr();
//		num = page.page();
//		RMPageHdr hdr(capacity_, addr);
//		hdr.setNext(PAGE_LIST_END);
//		hdr.setRemain(capacity_);
//		int remain = hdr.remain();
//		hdr.map.setAll();
//		page.setDirty();
//		rmhdr_.free = num; // �������ҳ���ҳ����ӵ�����������
//		rmhdr_.size++;
//		changed_ = true;
//	}

//	RMPageHdr hdr(capacity_, addr);
//	for (int i = 0; i < capacity_; i++) {
//		if (hdr.map.available(i)) { // ��λ��ǡ�ÿ���
//			slot = i;
//			return true;
//		}
//	}
//	return false; // unexpected error
//}




