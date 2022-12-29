#include "ql_stream.h"
#include "ix.h"
#include "ql_error.h"
using namespace std;

extern RMManager rmManager;
extern IX_Manager ixManager;

/*~~~~~~~~~~~~~~~~~~~~~~~~~IdxWrapper~~~~~~~~~~~~~~~~~~~~~~*/
IdxWrapper::IdxWrapper(const char* pathname, const DataAttr& idxattr, Operator op, const void* val,
	int nattrs, const DataAttr attrs[], int nconds, const Condition conds[])
	: Stream(nattrs, attrs, nconds, conds), pathname_(pathname)
{
	rmManager.openFile(pathname_, file_);
	ixManager.OpenIndex(pathname_, idxattr.idxno, index_); /* �������ļ� */
	scan_.OpenScan(index_, op, val);
}

RC IdxWrapper::open()
{
	/* Ԥ����ø������� */
	for (int i = 0; i < conds_.size(); i++) {
		DataAttr& attr = attrs_[indicator_[conds_[i].lhsAttr.attrname]];
		loffsets_.push_back(attr.offset); /* ��Ҫ��ƫ������ס,�����ѯ */
		comps_.push_back(make_comp(attr.type, attr.len));
	}
	return 0;
}

RC IdxWrapper::next(uint8_t* data)
{
	bool passed = false;
	RID rid;
	RMRecord rcd;
	RC errval;
	Ptr ptr;
	while (!passed) {
		errval = scan_.GetNextEntry(rid);
		if (errval == IX_EOF) return QL_EOF;
		//file_->getRcd(rid, rcd);
		char* addr;
		int len;
		file_->getRcd(rid.page(),rid.slot(),addr,len);
		rcd.set(addr, len, rid);
		ptr = rcd.rawPtr();
		int i = 1;
		/* ʹ�ø����������������� */
		for (; i < comps_.size(); i++) {
			if (!comps_[i]->eval(ptr + loffsets_[i], conds_[i].op, conds_[i].rhsValue.data)) break;
		}
		if (i == comps_.size()) passed = true;
	}
	memcpy(data, ptr, rcdlen_);
	return 0;
}

RC IdxWrapper::rewind()
{
	return scan_.rewind();
}

RC IdxWrapper::close()
{
	rmManager.closeFile(file_);
	ixManager.CloseIndex(index_);
	return 0;
}

IdxWrapper::~IdxWrapper()
{
	for (int i = 0; i < comps_.size(); i++) {
		delete comps_[i];
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~RcdWrapper~~~~~~~~~~~~~~~~~~~~~~*/
RC RcdWrapper::open()
{
	/* step 1 -> ���ļ���RMFileScan */
	rmManager.openFile(pathname_, file_);
	if (conds_.size() > 0) {
		/* ѡ���һ������ */
		DataAttr& attr = attrs_[indicator_[conds_[0].lhsAttr.attrname]];
		scan_.openScan(file_, attr.type, attr.len, attr.offset, conds_[0].op, conds_[0].rhsValue.data);
	}
	else {
		/* ֱ��ȫ����� */
		scan_.openScan(file_, INT0, sizeof(int), 0, NO_OP, nullptr);
	}

	/* step2 -> Ԥ��������������,�����0��ʼֻ��Ϊ�˷������,�������� */
	for (int i = 0; i < conds_.size(); i++) {
		DataAttr& attr = attrs_[indicator_[conds_[i].lhsAttr.attrname]];
		loffsets_.push_back(attr.offset);
		comps_.push_back(make_comp(attr.type, attr.len));
	}
	return 0;
}

//
// next - ��ȡ��һ����¼
// 
RC RcdWrapper::next(uint8_t* data)
{
	bool passed = false;
	RMRecord rcd;
	RC errval;
	Ptr ptr;
	while (!passed) {
		errval = scan_.getNextRcd(rcd);
		if (errval == RM_EOF) return QL_EOF;
		ptr = rcd.rawPtr();
		int i = 0;
		for (; i < comps_.size(); i++) {
			if (!comps_[i]->eval(ptr + loffsets_[i], conds_[i].op, conds_[i].rhsValue.data)) break;
		}
		if (i == conds_.size()) passed = true;
	}
	memcpy(data, ptr, rcdlen_);
	return 0;
}

//
// rewind - ���»ص����
// 
RC RcdWrapper::rewind()
{
	return scan_.rewind();
}

RC RcdWrapper::close()
{
	scan_.closeScan();
	rmManager.closeFile(file_);
	return 0;
}

RcdWrapper::~RcdWrapper()
{
	for (int i = 0; i < comps_.size(); i++) {
		delete comps_[i];
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~CombWrpperWrapper~~~~~~~~~~~~~~~~~~~~~~*/
RC CombWrapper::open()
{
	lhs_->open();
	rhs_->open();
	/* step 1 -> �ҵ�condition�����Ҳ�������Ӧ��attr */
	const char* lrel, * rrel, * lattr, * rattr;
	int idx;
	for (int i = 0; i < conds_.size(); i++) {
		lrel = conds_[i].lhsAttr.relname;
		rrel = conds_[i].rhsAttr.relname;
		lattr = conds_[i].lhsAttr.attrname;
		rattr = conds_[i].rhsAttr.attrname;

		for (int j = 0; j < attrs_.size(); j++) {
			if (strcmp(lattr, attrs_[j].attrname) == 0 &&
				strcmp(lrel, attrs_[j].relname) == 0) {
				loffsets_.push_back(attrs_[j].offset);
				idx = j;
			}
			else if (strcmp(rattr, attrs_[j].attrname) == 0 &&
				strcmp(rrel, attrs_[j].relname) == 0) {
				roffsets_.push_back(attrs_[j].offset);
			}
		}
		assert(loffsets_.size() == roffsets_.size());
		/* ����һ�������� */
		comps_.push_back(make_comp(attrs_[idx].type, attrs_[idx].len));
	}
	return 0;
}

//
// next - ��ȡ��һ����¼
// 
RC CombWrapper::next(uint8_t* data)
{
	bool passed = false;
	RC errval;
	uint8_t* rdata = data + lhs_->rcdlen_;
	/* �տ�ʼ��ʱ����Ҫ�趨һ�� */
	if (ldata_ == nullptr) {
		ldata_ = (uint8_t*)malloc(lhs_->rcdlen_);
		errval = lhs_->next(ldata_);
		if (errval == QL_EOF) return QL_EOF;
	}
	while (!passed) {
		/* �ȴ����Ҳ� */
		errval = rhs_->next(rdata);
		if (errval == QL_EOF) {
			rhs_->rewind();
			errval = rhs_->next(rdata);
			if (errval == QL_EOF) return QL_EOF;
			errval = lhs_->next(ldata_);
			if (errval == QL_EOF) return QL_EOF;
		}
		memcpy(data, ldata_, lhs_->rcdlen_);
		int i = 0;
		for (; i < comps_.size(); i++) {
			if (!comps_[i]->eval(data + loffsets_[i], conds_[i].op, data + roffsets_[i]))
				break;
		}
		if (i == comps_.size()) passed = true;
	}
	return 0;
}

RC CombWrapper::rewind()
{
	lhs_->rewind();
	rhs_->rewind();
	return 0;
}

RC CombWrapper::close()
{
	lhs_->close();
	rhs_->close();
	return 0;
}

CombWrapper::~CombWrapper()
{

	for (int i = 0; i < comps_.size(); i++) {
		delete comps_[i];
	}

	if (ldata_) free(ldata_);
}

void CombWrapper::clean()
{
	lhs_->clean();
	rhs_->clean();
	delete lhs_;
	delete rhs_;
}

