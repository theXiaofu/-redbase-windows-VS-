#include <limits.h>
#include <float.h>
#include <functional>
#include <tuple>
#include <string.h>
#include "sm_manager.h"
#include "ql_query.h"
#include "ql_error.h"
using namespace std;

extern SMManager smManager;
static Condition condspool[MAXCONDS];
using Conds = tuple<int, Condition*>;

//
// lookupConds - �����й���relname���ű�����е�һԪ����,relname��ʾ�������
//
static Conds lookupUnaryConds(const char* relname, int nconds, const Condition conditions[])
{
	int count = 0; /* һԪ��������Ŀ */
	for (int i = 0; i < nconds; i++) {
		if (!conditions[i].bRhsIsAttr &&
			strcmp(conditions[i].lhsAttr.relname, relname) == 0) {
			condspool[count++] = conditions[i];
		}
	}
	return Conds(count, condspool);
}

//
// lookupConds - �������еĶ�Ԫ����
//
static Conds lookupBinaryConds(int nconds, const Condition conditions[])
{
	int count = 0; /* ��Ԫ��������Ŀ */
	for (int i = 0; i < nconds; i++) {
		if (conditions[i].bRhsIsAttr) { /* ���Ҳ������������ԣ����˶�Ԫ */
			condspool[count++] = conditions[i];
		}
	}
	return Conds(count, condspool);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sort~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static bool int_comp_desc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	int lval = *(int*)((uint8_t*)lhs + offset), rval = *(int*)((uint8_t*)rhs + offset);
	return lval > rval;
}

static bool int_comp_asc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	int lval = *(int*)((uint8_t*)lhs + offset), rval = *(int*)((uint8_t*)rhs + offset);
	return lval < rval;
}

static bool float_comp_desc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	int lval = *(float*)((uint8_t*)lhs + offset), rval = *(float*)((uint8_t*)rhs + offset);
	return lval > rval;
}

static bool float_comp_asc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	int lval = *(float*)((uint8_t*)lhs + offset), rval = *(float*)((uint8_t*)rhs + offset);
	return lval < rval;
}

static bool str_comp_desc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	char* lval = (char*)lhs + offset, * rval = (char*)rhs + offset;
	return strncmp(lval, rval, comp_len) > 0;
}

static bool str_comp_asc(const void* lhs, const void* rhs, int offset, int comp_len)
{
	char* lval = (char*)lhs + offset, * rval = (char*)rhs + offset;
	return strncmp(lval, rval, comp_len) < 0;
}

//
// swap - ����array��i��jλ�õ�Ԫ��
//
static void inline swap(vector<uint8_t*>& array, int i, int j)
{
	uint8_t* tmp = array[j];
	array[j] = array[i];
	array[i] = tmp;
}
//����
static int partition(vector<uint8_t*>& array, int left, int right, SortConf& conf)
{
	int pivot_idx = right;
	uint8_t* pivot = array[pivot_idx];	/* ѡ�����ĵ�Ԫ����Ϊpivot */
	int lidx = left, ridx = right - 1;

	for (; ;) {
		while (conf.func(array[lidx], pivot, conf.offset, conf.comp_len) && lidx < right) lidx++;
		while (!conf.func(array[ridx], pivot, conf.offset, conf.comp_len) && ridx > lidx) ridx--;

		if (lidx < ridx)
			swap(array, lidx, ridx);
		else break;
	}
	swap(array, right, lidx);
	return lidx;
}

//
// sort - ʵ�ֿ�������
//
static void sort(vector<uint8_t*>& array, int left, int right, SortConf& conf)
{
	if (left >= right) return;
	int pivot_idx = partition(array, left, right, conf);
	sort(array, left, pivot_idx - 1, conf);
	sort(array, pivot_idx + 1, right, conf);
}

static void data_sort(vector<uint8_t*>& datas, SortConf& conf)
{
	sort(datas, 0, datas.size() - 1, conf);
}

//
// pick_comp_fun - ��ѡ�����ʵ�������,order��ʾ������˳��type��ʾԪ�ص�����
//
static comp_func pick_comp_fun(int order, AttrType type)
{
	comp_func func;
	switch (type)
	{
	case INT0: func = order > 0 ? int_comp_asc : int_comp_desc; break;
	case FLOAT0: func = order > 0 ? float_comp_asc : float_comp_desc; break;
	default: func = order > 0 ? str_comp_asc : str_comp_desc; break;
	}
	return func;
}


Query* query_new(int nselattrs, const AggRelAttr selattrs[], int nrelations,
	const char* const relations[], int nconditions, const Condition conditions[],
	int order, RelAttr& orderattr, bool group, RelAttr& groupattr)
{
	int nattrs, nconds;
	Condition* unary_conds, * binary_conds; /* һ������ֻ����һԪ�����Ͷ�Ԫ���� */
	DataAttr attrs[MAXATTRS];
	map<const char*, Stream*, CharCmp> rels;
	/* step1.1 -> �����ű� */
	for (int i = 0; i < nrelations; i++) { /* nrelations�����Ŀ */
		Stream* stream;
		DataAttr* idxattr = nullptr;
		/* �������ű���������� */
		smManager.lookupAttrs(relations[i], nattrs, attrs);
		/* �����й������ű������һԪ���� */
		tie(nconds, unary_conds) = lookupUnaryConds(relations[i], nconditions, conditions);

		/* ����Ƿ��ܹ�ʹ������,�������ֻ��ʹ��һ������ */
		int idx = 0;
		for (int j = 0; j < nattrs; j++) {
			if (attrs[j].idxno == -1) continue;
			for (; idx < nconds; idx++) {
				if (strcmp(unary_conds[idx].lhsAttr.attrname, attrs[j].attrname) == 0) {
					idxattr = attrs + j;
					break;
				}
			}
		}

		if (idxattr != nullptr) { /* ����ʹ������ */
			Operator op = unary_conds[idx].op;
			void* data = unary_conds[idx].rhsValue.data;
			memmove(&unary_conds[idx], &unary_conds[idx + 1], sizeof(Condition) * (nconds - idx));
			stream = new IdxWrapper(relations[i], *idxattr, op, data,
				nattrs, attrs, nconds - 1, unary_conds);
		}
		else { /* ������ʹ������ */
			stream = new RcdWrapper(relations[i], nattrs, attrs, nconds, unary_conds);
		}
		/* �������������뵽������֮�� */
		rels.emplace(relations[i], stream);
	}

	/* step 1.2 -> ��ʼ���ӱ�,Ҳ������ν�Ĳ�� */
	Stream* comb = rels[relations[0]];
	for (int i = 1; i < nrelations; i++) {
		Stream* rs = rels[relations[i]];
		comb = new CombWrapper(comb, rs); /* ��ͣ�ع����������� */
	}
	/* �������еĶ�Ԫ����,��Ԫ����һ����ԣ�ֻ�ǹ��������� */
	tie(nconds, binary_conds) = lookupBinaryConds(nconditions, conditions);
	/* �����µĶ�Ԫ����ȫ�������stream�� */
	for (int i = 0; i < nconds; i++) {
		comb->appendCond(binary_conds[i]);
	}
	return new Query(comb, nselattrs, selattrs, order, orderattr, group, groupattr);
}

void query_free(Query* query)
{
	query->clean();
	delete query;
}

//
// rcd_comp - ���ڱȽ�������¼
//
static int rcd_comp(void* lhs, void* rhs, void* len, void* extra)
{
	return memcmp(lhs, rhs, *(int*)len);
}

//
// rcd_attr_comp - ���ڱȽ�������¼��ĳ������
//
static int rcd_attr_comp(void* lhs, void* rhs, void* off, void* len)
{
	int offset = *(int*)off;
	uint8_t* l = (uint8_t*)lhs + offset;
	uint8_t* r = (uint8_t*)rhs + offset;
	return memcmp(l, r, *(int*)len);
}

Query::Query(Stream* stream, int nselattrs, const AggRelAttr selattrs[],
	int order, RelAttr& orderattr, bool group, RelAttr& groupattr)
	: stream_(stream), aggs_(selattrs, selattrs + nselattrs)
	, collected_(false), pos_(0), containAgg_(false), eof_(false)
	, group_(group), order_(order)
	, groupattrLen_(0), groupattrOffset_(0), grouppos_(0), rcdlen_(0)
{
	/* ��ʱ����������,�ȴ������, Ҫ����Ļ�,����Ҫ��map,��סĳ�����Ե�ֵ���ֹ�û�� */
	/* ������Ҫ��stream����ѡһЩ����,����ס���ǵ�����  */
	auto& attrs = stream->attrs_;
	/* star��ʾѡ�����е����� */
	ItemDiscrip disc;
	if (nselattrs == 1 && selattrs[0].func == NO_F &&
		strcmp(selattrs[0].attrname, "*") == 0) {
		disc.func = NO_F;
		for (int i = 0; i < attrs.size(); i++) {
			auto& attr = attrs[i];
			strcpy(disc.relname, attr.relname);
			strcpy(disc.attrname, attr.attrname);
			disc.len = attr.len;
			disc.type = attr.type;
			attrs_.push_back(attrs[i]);
			discs_.push_back(disc);
		}
		rcdlen_ = stream->rcdlen_;
	}
	else {
		for (int i = 0; i < nselattrs; i++) {
			if (selattrs[i].func != NO_F) containAgg_ = true;  /* �ڸ������ϴ��ھۼ����� */
			disc.func = selattrs[i].func;
			strcpy(disc.attrname, selattrs[i].attrname);
			if (selattrs[i].relname == nullptr)
				memset(disc.relname, 0, MAXNAME + 1);
			else
				strcpy(disc.relname, selattrs[i].relname);

			/* count(*)��Ҫ���⴦��һ�� */
			if (selattrs[i].func == COUNT_F &&
				strcmp(selattrs[i].attrname, "*") == 0) {
				disc.type = INT0;
				disc.len = sizeof(int);
				rcdlen_ += disc.len;
				discs_.push_back(disc);
				/* ��attrs�л�Ҫ����һ������ֵ,���ֵʵ�ʲ����õ� */
				attrs_.push_back(attrs[0]);
				continue;
			}


			for (int j = 0; j < attrs.size(); j++) {
				if (strcmp(selattrs[i].attrname, attrs[j].attrname) == 0 &&
					(selattrs[i].relname == nullptr ||
						/* selattrs��relname����Ϊ�� */
						strcmp(selattrs[i].relname, attrs[j].relname) == 0)) {
					attrs_.push_back(attrs[j]);
					switch (selattrs[i].func)
					{
					case AVG_F:
						disc.type = FLOAT0, disc.len = sizeof(float); break;
					case COUNT_F:
						disc.type = INT0; disc.len = sizeof(int); break;
					default:
						disc.type = attrs[j].type; disc.len = attrs[j].len; break;
					}
					rcdlen_ += disc.len;
					discs_.push_back(disc);
				}
			}
		}
	}
	/* �����group��order */
	if (order || group) {
		for (int i = 0; i < attrs_.size(); i++) {
			if (order && (strcmp(attrs_[i].attrname, orderattr.attrname) == 0) &&
				(orderattr.relname == nullptr ||
					strcmp(orderattr.relname, attrs_[i].relname) == 0)) {
				conf_.comp_len = attrs_[i].len;
				conf_.offset = attrs_[i].offset;
				conf_.func = pick_comp_fun(order, attrs_[i].type);
			}
			else if (group && (strcmp(attrs_[i].attrname, groupattr.attrname) == 0) &&
				(groupattr.relname == nullptr ||
					strcmp(orderattr.relname, attrs_[i].relname) == 0)) {
				groupattrLen_ = attrs_[i].len;
				groupattrOffset_ = attrs_[i].offset;
			}
		}
	}
	printeddatas_ = avl_map_new(rcd_comp, &rcdlen_, nullptr);
}

Query::~Query()
{
	/* �ͷ��ڴ� */
	for (int i = 0; i < groupdatas_.size(); i++) {
		for (int j = 0; j < groupdatas_[i].size(); j++) {
			free(groupdatas_[i][j]);
		}
	}
	avl_map_traverse(printeddatas_, free); /* �ͷ��ڴ� */
	avl_map_free(printeddatas_);
}

RC Query::next(Item& it)
{
	/* step 1 -> ��һ�ε��õ�ʱ��,��Ҫ�����еļ�¼���������� */
	RC errval;
	uint8_t* data;
	int len = stream_->rcdlen_;
	if (!collected_) {
		int idx = 0;
		for (; ; ) {
			data = (uint8_t*)malloc(len);
			errval = stream_->next(data);
			if (errval == QL_EOF) {
				free(data);
				break;
			}
			idx = chooseGroup(data);
			groupdatas_[idx].push_back(data);
		}
		collected_ = true;
	}

	/* step 2 -> ��ʼ����ۼ�����,ͶӰҪ������޷�����,һ���ǰ����ۼ�������,����һ���ǲ������� */
	if (!eof_) {
		bool passed = false;
		while (!passed) {
			data = (uint8_t*)malloc(rcdlen_);
			if (containAgg_) {
				int offset = 0;
				for (int g = grouppos_; g < groupdatas_.size(); g++) {
					for (int i = 0; i < aggs_.size(); i++) {
						switch (aggs_[i].func)
						{
						case MAX_F: handleMax(groupdatas_[g], attrs_[i], data + offset); break;
						case MIN_F: handleMin(groupdatas_[g], attrs_[i], data + offset); break;
						case SUM_F: handleSum(groupdatas_[g], attrs_[i], data + offset); break;
						case AVG_F: handleAvg(groupdatas_[g], attrs_[i], data + offset); break;
						case COUNT_F:
						{
							int count = groupdatas_[g].size();
							memcpy(data + offset, &count, sizeof(int));
							break;
						}
						default:
							memcpy(data + offset, groupdatas_[g][i] + attrs_[i].offset, attrs_[i].len);
							break;
						}
						offset += discs_[i].len;
					}
				}
				grouppos_++;
			}
			else { /* ����������ۼ����� */
				int offset = 0;
				auto& datas = groupdatas_[grouppos_];
				/* ÿһ��������֮ǰ��Ҫ�������� */
				if (pos_ == 0 && grouppos_ != groupdatas_.size() && order_) {
					data_sort(datas, conf_);
				}
				for (int i = 0; i < attrs_.size(); i++) {
					memcpy(data + offset, datas[pos_] + attrs_[i].offset, attrs_[i].len);
					offset += attrs_[i].len;
				}
				pos_++;
				if (pos_ % datas.size() == 0) {
					pos_ = 0;
					grouppos_++;
				}
			}
			if (grouppos_ == groupdatas_.size()) eof_ = true;
			/* ȥ���ظ� */
			if (avl_map_insert(printeddatas_, data, data)) {
				it.data = data;
				it.rcdlen = rcdlen_;
				passed = true;
			}
			else {
				free(data);
				if (eof_) return QL_EOF;
			}
		}
		return 0;
	}

	return QL_EOF;
}

//
// handleMax - ��ĳһ�е����ֵ
//
void Query::handleMax(vector<uint8_t*>& datas, DataAttr& attr, uint8_t* data)
{
	if (attr.type == INT0) {
		DataComp<int> comp(attr.offset);
		comp.findMax(datas, data);
	}
	else {
		DataComp<float> comp(attr.offset);
		comp.findMax(datas, data);
	}
}

void Query::handleMin(vector<uint8_t*>& datas, DataAttr& attr, uint8_t* data)
{
	if (attr.type == INT0) {
		DataComp<int> comp(attr.offset);
		comp.findMin(datas, data);
	}
	else {
		DataComp<float> comp(attr.offset);
		comp.findMin(datas, data);
	}
}

void Query::handleAvg(vector<uint8_t*>& datas, DataAttr& attr, uint8_t* data)
{
	if (attr.type == INT0) {
		DataComp<int> comp(attr.offset);
		comp.calcAvg(datas, data);
	}
	else {
		DataComp<float> comp(attr.offset);
		comp.calcAvg(datas, data);
	}
}

void Query::handleSum(vector<uint8_t*>& datas, DataAttr& attr, uint8_t* data)
{
	if (attr.type == INT0) {
		DataComp<int> comp(attr.offset);
		comp.calcSum(datas, data);
	}
	else {
		DataComp<float> comp(attr.offset);
		comp.calcSum(datas, data);
	}
}

//
// chooseGroup - ����dataѡ�����ʵ���
// 
int Query::chooseGroup(uint8_t* data)
{
	if (group_) {
		for (int i = 0; i < groupdatas_.size(); i++) {
			auto ptr = groupdatas_[i][0];
			if (memcmp(data + groupattrOffset_, ptr + groupattrOffset_, groupattrLen_) == 0)
				return i;
		}
	}
	else if (groupdatas_.size() != 0) return 0;
	groupdatas_.emplace_back();
	return groupdatas_.size() - 1;
}

void Query::clean()
{
	stream_->clean();
	delete stream_;
}