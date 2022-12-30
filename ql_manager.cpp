#include <algorithm>
#include "redbase.h"
#include "ql_manager.h"
#include "parser_interp.h"
#include "ql_error.h"
#include "printer.h"
#include "ql_query.h"
#include <vector>
using namespace std;

#define MAXDATAATTR		(3 * MAXATTRS)
#define MAXCONDITIONS	(2 * MAXATTRS)
#define MAXCHAR			1024
/*~~~~~~~~~~~~~~~~~~~~~~~~~~Utilitys~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
extern PFManager pfManager;
extern RMManager rmManager;
extern IX_Manager ixManager;
extern SMManager smManager;

static char buffer[1024];

static DataAttr attrs[MAXDATAATTR];

//
// insert - �����ݲ��뵽relname���ű���
//
RC QLManager::insert(const char* relname, int nvals, Value vals[])
{
	RC errval;
	int nattrs;
	/* �����ʱ����һ��������Ҫ���,�Ǿ�����ҪУ�������Ƿ���Ϲ淶 */
	smManager.lookupAttrs(relname, nattrs, attrs);
	if (nattrs != nvals) {
		return QL_INVALIDSIZE;
	}

	int size = 0;
	for (int i = 0; i < nvals; i++) {
		if (vals[i].type != attrs[i].type) return QL_JOINKEYTYPEMISMATCH;
		size += attrs[i].len;
	}
	int offset = 0;
	for (int i = 0; i < nvals; i++) {
		memcpy(buffer + offset, vals[i].data, attrs[i].len);
		offset += attrs[i].len;
	}
	RMFilePtr file;
	RID rid;
	rmManager.openFile(relname, file);
	//errval = file->insertRcd(buffer, rid);
	int page0;
	int slot0;
	file->insertRcd(buffer, sizeof(buffer), page0, slot0);
	rid.set(page0, slot0);
	rmManager.closeFile(file);
	return 0;
}

//
// select : ����select����
// 
RC QLManager::select(int nselattrs, const AggRelAttr selattrs[],
	int nrelations, const char* const relations[],
	int nconditions, const Condition conditions[],
	int order, RelAttr orderattr, bool group, RelAttr groupattr)
{
	Query* query = query_new(nselattrs, selattrs, nrelations, relations,
		nconditions, conditions, order, orderattr, group, groupattr);
	Printer print(query->discs(), query->discsSize()); /* Printer���帺����� */
	query->open();
	Item it;
	RC errval;
	print.printHeader();
	for (; ; ) {
		errval = query->next(it);
		if (errval == QL_EOF) break;
		print.print(it.data);
	}
	print.printFooter();
	query->close();
	query_free(query);
	return 0;
}

RC QLManager::Delete( char* relname, int nconditions, const Condition conditions[])
{
	int nselattrs = 0;
	AggRelAttr selattrs[MAXATTRS];
	DataAttr attrs[MAXATTRS];
	smManager.lookupAttrs(relname, nselattrs, attrs);
	for (int i = 0; i < nselattrs; ++i)
	{
		selattrs[i].relname = attrs[i].relname;
		selattrs[i].attrname = attrs[i].attrname;
		selattrs[i].func = NO_F;
	}
	int nrelations = 1; /* �����Ŀ */
	char* relations[MAXATTRS]; /* ������� */
	relations[0] = relname;
	int order = 0; /* ����or���� */
	RelAttr orderattr; /* ����orderAttr������ */
	bool group = false;
	RelAttr groupattr; /* ����groupAttr������ */
	Query* query = query_new(nselattrs, selattrs, nrelations, relations,
		nconditions, conditions, order, orderattr, group, groupattr);
	Printer print(query->discs(), query->discsSize()); /* Printer���帺����� */
	query->open();
	Item it;
	RC errval;
	print.printHeader();
	RMFilePtr rmf;
	//RID rid;
	vector<RID> rids;
	//rmManager.openFile(relname,rmf);
	for (; ; ) {
		errval = query->next(it);
		//it.rid;
		//rmf->deleteRcd(it.rid);
		rids.push_back(it.rid);
		if (errval == QL_EOF) break;
		print.print(it.data);
	}

	print.printFooter();
	query->close();
	query_free(query);
	rmManager.openFile(relname, rmf);
	for (int i = 0; i < rids.size(); ++i)
	{
		rmf->deleteRcd(rids[i]);
	}
	rmManager.closeFile(rmf);
	return 0;
	//relAttrs
}

