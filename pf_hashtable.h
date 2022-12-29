//
// File:		pf_hashtable.h
// Description: PFHashTable class interface
//

#ifndef PF_HASHTABLE_H
#define PF_HASHTABLE_H

#include "pf_internal.h"
#include <list>
#include <vector>
using namespace std;


//
// PFHashTable - һ���ǳ����׵�hash��ʵ��,����������˵,�򵥵���Щ����˼��,
// ���table��¼��һЩʲô��?���������ʵ����һ��map,��¼��fdָ��������ļ��ĵ�num��ҳ��
// �������Ǹ�slot����.
//
class PFHashTable {
	struct Triple {
		FILE* fd;
		Page num;
		int slot;
		Triple(FILE* fd, Page num, int slot) : fd(fd), num(num), slot(slot) {}
	};
public:
	PFHashTable(uint capacity);
	~PFHashTable() {}
public:
	int search(FILE* fd, Page num);
	bool insert(FILE* fd, Page num, int slot);
	bool remove(FILE* fd, Page num);
private:
	int calcHash(FILE* fd, Page num)
	{
		return ((int)fd + num) % capacity_;
	}
private:
	uint capacity_;
	vector<list<Triple>> table_;//��ϣ����ߴ洢�����
};

#endif /* PF_HASHTABLE_H */