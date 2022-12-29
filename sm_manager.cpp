#include <string.h>
#include <stddef.h>
#include <set>
#include <fstream>
//#include <iostream>
#include "sm_manager.h"
#include "sm_error.h"
#include "rm_filescan.h"
#include "util.h"
#include "catalog.h"
#include "printer.h"
#include "data_attr.h"
#include "parser_interp.h"
using namespace RedBase;
using namespace std;

 extern RMManager rmManager;
 //PFManager pfm;
 extern IX_Manager ixManager;
static char _buffer[1024];
static DataAttr _attrs[MAXATTRS];

//
// getline - ���ļ��ж�ȡһ������
// 
static char* getline(FILE* fd)
{
	int count = 0;
	for (; ; ) {
		int n = Read(fd,&_buffer[count], 1);
		//_buffer[count] = fgetc(fd);
		if (n == 0) {
			if (count == 0) return nullptr;
			else break;
		}
		if (_buffer[count] == '\n') {
			_buffer[count] = '\0';
			break;
		}
		count++;
	}
	return _buffer;
}

//
// getitem - ��ȡһ���е�һ��
// 
static char* getitem(char*& line)
{
	int i;
	char* ptr = line;
	for (i = 0; line[i] != ',' && line[i] != '\0'; i++);
	line[i] = '\0';
	line += i + 1;
	return ptr;
}

//
// openDb - �����ݿ�
// 
RC SMManager::openDb(const char* dbpath)
{
	if (dbpath == nullptr) return SM_BADOPEN;
	Getcwd(workdir_, 1024);
	Chdir(dbpath);
	rmManager.openFile("attrcat", attr_);
	rmManager.openFile("relcat", rel_);
	opened_ = true;
	return 0;
}

//
// closeDb - �ر����ݿ�
// 
RC SMManager::closeDb()
{
	if (!opened_) return SM_NOTOPEN;
	rmManager.closeFile(attr_);
	rmManager.closeFile(rel_);
	Chdir(workdir_);
	opened_ = false;
	return 0;
}

//
// createTable - ������
// 
RC SMManager::createTable(const char* relname, int count, AttrInfo* infos)
{
	if (count <= 0 || infos == nullptr)
		return SM_BADTABLE;

	if ((strcmp(relname, "relcat") == 0) || (strcmp(relname, "attrcat") == 0))
		return SM_BADTABLE;

	// �кܶ����Ӧ�õ�����������,����˵�������Ƶ��ظ�,��Ӧ������parse��ʱ���Ӧ�ñ���
	// ����Ӧ�õ�����������
	int offset = 0;
	RID rid;
	DataAttr attr;
	strcpy(attr.relname, relname);
	for (int i = 0; i < count; i++) {
		attr.type = infos[i].type;
		attr.len = infos[i].len;
		attr.idxno = -1;			// ��û����ʽ��������֮ǰ,idxno����-1
		attr.offset = offset;
		strcpy(attr.attrname, infos[i].attrname);
		offset += infos[i].len;
		int page0;
		int slot0;
		attr_->insertRcd(reinterpret_cast<Ptr>(&attr), sizeof(DataAttr), page0, slot0);
		rid.set(page0, slot0);
	}

	rmManager.createFile(relname, 1<<20);	// Ϊ����һ���ļ�
	DataRel rel;
	strcpy(rel.relname, relname);
	rel.attrcount = count;
	rel.pages = 1;			// ҳ����Ŀ
	rel.rcds = 0;			// ��¼����Ŀ
	//rel_->insertRcd(reinterpret_cast<Ptr>(&rel), rid);
	int page0;
	int slot0;
	rel_->insertRcd(reinterpret_cast<Ptr>(&rel), sizeof(DataRel), page0, slot0);
	int len;
	char* s0;
	//rel_->getRcd(page0, slot0, s0, len);
	//printf("%d %d len:%d\n", page0,slot0,len);
	rid.set(page0, slot0);
	return 0;
}

RC SMManager::dropTable(const char* relname)
{
	if (strcmp(relname, "relcat") == 0 || strcmp(relname, "attrcat") == 0)
		return SM_BADTABLE;

	RMFileScan scan;
	RMRecord rcd;
	Ptr val = const_cast<Ptr>(relname);
	RC errval = scan.openScan(rel_, STRING0, MAXNAME + 1, offsetof(DataRel, relname), EQ_OP, val);
	bool found = false;
	for (; ; ) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		Ptr addr = rcd.rawPtr();
		if (strcmp(reinterpret_cast<DataRel*>(addr)->relname, relname) == 0) {
			found = true;
			break;
		}
	}
	scan.closeScan();
	if (!found) return SM_NOSUCHTABLE;
	rmManager.destroyFile(relname);
	rel_->deleteRcd(rcd.rid());

	// ������Ҫɾ������
	DataAttr* attr;
	scan.openScan(attr_, STRING0, MAXNAME + 1, offsetof(DataAttr, relname), EQ_OP, val);
	for (; ; ) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		attr = reinterpret_cast<DataAttr*>(rcd.rawPtr());
		if (attr->idxno != -1) {
			dropIndex(relname, attr->attrname);
		}
		attr_->deleteRcd(rcd.rid());
	}
	scan.closeScan();
	return 0;
}

//
// createIndex - Ϊ���е�ĳ�����Թ���һ������
// 
RC SMManager::createIndex(const char* relname, const char* attrname)
{
	DataAttr attr;
	RID rid;
	RC errval = lookupAttr(relname, attrname, attr, rid);
	if (errval != 0) return errval;
	if (attr.idxno != -1) return SM_INDEXEXISTS;	// �����Ѿ�������
	attr.idxno = attr.offset;
	ixManager.CreateIndex(relname, attr.idxno, attr.type, attr.len);
	RMRecord rcd;
	IX_IndexHandle index;
	RMFilePtr file;
	rcd.set(reinterpret_cast<Ptr>(&attr), attr.len, rid);
	ixManager.OpenIndex(relname, attr.idxno, index);
	rmManager.openFile(relname, file);
	RMFileScan scan;
	Ptr addr;
	scan.openScan(file, attr.type, attr.len, attr.offset, NO_OP, nullptr);
	for (; ; ) {
		RMRecord rcd;
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		if (errval != 0) return errval;
		addr = rcd.rawPtr();
		index.InsertEntry(addr + attr.offset, rcd.rid());
	}

	scan.closeScan();
	ixManager.CloseIndex(index);
	return 0;
}

//
// dropIndex - ɾ��ĳ�������ϵ�����
//
RC SMManager::dropIndex(const char* relname, const char* attrname)
{
	DataAttr attr;
	RID rid;
	RC errval = lookupAttr(relname, attrname, attr, rid);
	if (errval != 0) return errval;
	if (attr.idxno == -1) return SM_INDEXNOTEXISTS;
	ixManager.DestroyIndex(relname, attr.idxno);
	attr.idxno = -1;
	RMRecord rcd;
	int page0, slot0;
	rcd.set(reinterpret_cast<Ptr>(&attr), sizeof(DataAttr), rid);
	attr_->updateRcd(rcd);
	//attr_->deleteRcd(rcd.rid());
	//attr_->insertRcd()
	return 0;
}

RC SMManager::lookupAttr(const char* relname, const char* attrname, DataAttr& attr, RID& rid)
{
	RMFileScan scan;
	RMRecord rcd;
	DataAttr* addr;
	Ptr val = const_cast<Ptr>(relname);
	RC errval = scan.openScan(attr_, STRING0, MAXNAME + 1, offsetof(DataAttr, relname),
		EQ_OP, val);

	bool exists = false;
	for (;;) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		addr = reinterpret_cast<DataAttr*>(rcd.rawPtr());
		if (strcmp(addr->attrname, attrname) == 0) {
			exists = true;
			attr = *addr;
			rid = rcd.rid();
			break;
		}
	}
	scan.closeScan();
	if (!exists) return SM_BADATTR;
	return 0;
}

//
// lookupRel - ��relcat���в�ѯ�й���relname���ű�������Ϣ
//
RC SMManager::lookupRel(const char* relname, DataRel& rel, RID& rid)
{
	RMFileScan scan;
	RC errval = scan.openScan(rel_, STRING0, MAXNAME + 1,
		offsetof(DataRel, relname), EQ_OP, const_cast<char*>(relname));
	RMRecord rcd;
	scan.getNextRcd(rcd);
	if (errval == RM_EOF) return SM_NOSUCHTABLE;
	DataRel* addr = reinterpret_cast<DataRel*>(rcd.rawPtr());
	rel = *addr;
	rid = rcd.rid();
	scan.closeScan();
	return 0;
}

//
// lookupAttrs - ��ѯĳ�ű����е�������Ϣ
//
RC SMManager::lookupAttrs(const char* relname, int& nattrs, DataAttr attrs[])
{
	RC errval;
	RMFileScan scan;
	Ptr val = const_cast<char*>(relname);
	DataAttr* attr;
	//RMFilePtr file;
	scan.openScan(attr_, STRING0, MAXNAME + 1, offsetof(DataRel, relname), EQ_OP, val);
	RMRecord rcd;
	nattrs = 0;
	for (;;) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		attr = reinterpret_cast<DataAttr*>(rcd.rawPtr());
		attrs[nattrs] = *attr;
		nattrs++;
	}
	scan.closeScan();
	return 0;
}


RC SMManager::load(const char* relname, const char* filename)
{
	RMFilePtr file;
	RC errval = 0;
	char cache[MAXSTRINGLEN];
	int nattrs = 0, nlines = 0;
	char* line;
	rmManager.openFile(relname, file);
	//int fd = open(filename, O_RDONLY);
	FILE* fd = Open(filename, "rb+");
	//if ((int)( * fd) < 0) return SM_BADTABLE;
	errval = lookupAttrs(relname, nattrs, _attrs);
	if (errval < 0) return errval;

	// �����е�����,��Ϊ����Ҫͬʱ��������
	vector<IX_IndexHandle> indexes(nattrs);
	int offset = 0;
	for (int i = 0; i < nattrs; i++) {
		offset += _attrs[i].len;
		if (_attrs[i].idxno != -1) {
			//system("pause");
			//printf("%d\n", _attrs->idxno);
			ixManager.OpenIndex(relname, _attrs[i].idxno, indexes[i]);
		}
		//else
			//indexes.pop_back();
	}
	while ((line = getline(fd)) != nullptr) {
		char* item;
		//printf("%s\n", item);
		for (int i = 0; i < nattrs; i++) {
			item = getitem(line);
			//printf("%s\n", item);
			switch (_attrs[i].type)
			{
			case INT0:
			{
				int val;
				sscanf(item, "%d", &val);
				memcpy(cache + _attrs[i].offset, &val, _attrs[i].len);
				break;
			}
			case FLOAT0:
			{
				float val;
				sscanf(item, "%f", &val);
				memcpy(cache + _attrs[i].offset, &val, _attrs[i].len);
				break;
			}
			case STRING0:
			{
				memcpy(cache + _attrs[i].offset, item, _attrs[i].len);
				break;
			}
			default:
				break;

			}
		}
		RID rid;
		int page0;
		int slot0;
		//attr_->insertRcd(reinterpret_cast<Ptr>(&attr), sizeof(DataAttr), page0, slot0);
		file->insertRcd(cache,sizeof(cache),page0,slot0);
		rid.set(page0, slot0);
		
		for (int i = 0; i < indexes.size(); i++) {
			//if (indexes[i])
			//system("pause");
			if(_attrs[i].idxno!=-1)
				indexes[i].InsertEntry(cache + _attrs[i].offset, rid);
		}
		nlines++;
	}

	DataRel rel;
	RID rid;
	lookupRel(relname, rel, rid);
	rel.rcds += nlines;
	rel.pages = file->pagesSize();
	RMRecord rcd;
	rcd.set((Ptr)&rel, DataRel::spaceUsage(), rid);
	rel_->updateRcd(rcd);
	rmManager.closeFile(file);
	for (int i = 0; i < nattrs; i++) {
		 ixManager.CloseIndex(indexes[i]);
	}
	Close(fd);
	return 0;
}


RC SMManager::loadFromTable(const char* relname, int& count, DataAttr*& attrs)
{
	if (relname == nullptr) return SM_NOSUCHTABLE;
	RMFileScan scan;
	RC errval = scan.openScan(rel_, STRING0, MAXNAME + 1, offsetof(DataAttr, relname), EQ_OP,
		const_cast<char*>(relname));
	RMRecord rcd;
	scan.getNextRcd(rcd);
	if (errval == RM_EOF) return SM_NOSUCHTABLE;

	DataRel* rels = reinterpret_cast<DataRel*>(rcd.rawPtr());
	scan.closeScan();
	count = rels->attrcount;
	attrs = new DataAttr[count];
	RMFileScan s2;
	s2.openScan(attr_, STRING0, MAXNAME + 1, offsetof(DataAttr, relname), EQ_OP,
		const_cast<char*>(relname));

	int i;
	for (i = 0; i < count; i++) {
		RMRecord rcd;
		errval = s2.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		attrs[i] = *reinterpret_cast<DataAttr*>(rcd.rawPtr());
	}

	if (i < count) return SM_BADTABLE;
	s2.closeScan();
	return 0;
}

//
// help - ������еı�
// 
RC SMManager::help()
{
	RMRecord rcd;
	Ptr addr;
	DataAttr attr;
	strcpy(attr.relname, "relcat");
	strcpy(attr.attrname, "relname");
	attr.offset = offsetof(DataRel, relname);
	attr.type = STRING0;
	attr.len = MAXNAME + 1;

	Printer p(&attr, 1); // ֻ��Ҫ������еı�����Ƽ���
	p.printHeader();
	RMFileScan scan;
	RC errval = scan.openScan(rel_, INT0, sizeof(int), 0, NO_OP, nullptr);

	for (;;) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		addr = rcd.rawPtr();
		p.print((uint8_t*)addr);
	}
	p.printFooter();
	scan.closeScan();
	return 0;
}

//
// help - �����������,�����վ�������е�����
// 
RC SMManager::help(char* relname)
{
	int nattrs;
	RC errval = lookupAttrs("attrcat", nattrs, _attrs); // ��ȡattrcat���ű����������
	DataAttr* cleanedAttrs = &_attrs[nattrs];
	DataAttr except;
	int count = 0;
	RMRecord rcd;

	for (int i = 0; i < nattrs; i++) {
		if (strcmp(_attrs[i].attrname, "relname") != 0) {
			cleanedAttrs[count++] = _attrs[i];
		}
		else {
			except = _attrs[i];
		}
	}
	Printer output(cleanedAttrs, nattrs - 1);
	output.printHeader();
	RMFileScan scan;
	scan.openScan(attr_, STRING0, except.len, except.offset, EQ_OP, relname);
	for (; ; ) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		output.print((uint8_t*)rcd.rawPtr());
	}
	output.printFooter();
	scan.closeScan();
	return 0;
}

//
// print - ���ָ���������м�¼
// 
RC SMManager::print(const char* relname)
{
	RMFilePtr file;
	int nattrs;
	RC errval = rmManager.openFile(relname, file);
	lookupAttrs(relname, nattrs, _attrs);
	Printer print(_attrs, nattrs);
	print.printHeader();
	RMFileScan scan;
	scan.openScan(file, INT0, sizeof(int), 0, NO_OP, nullptr);
	RMRecord rcd;
	Ptr data;
	for (;;) {
		errval = scan.getNextRcd(rcd);
		if (errval == RM_EOF) break;
		data = rcd.rawPtr();
		print.print((uint8_t*)data);
	}
	print.printFooter();
	scan.closeScan();
	return 0;
}
