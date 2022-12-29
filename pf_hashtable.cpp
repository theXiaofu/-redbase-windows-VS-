#include "pf_internal.h"
#include "pf_hashtable.h"

PFHashTable::PFHashTable(uint capacity)
	: capacity_(capacity)
	, table_(capacity)
{
}

//
// search - �ڱ�����Ѱ��������Ԫ��,�ҵ���,���ؼ���,����Ļ�,����-1
// 
int PFHashTable::search(FILE* fd, Page num)
{
	int key = calcHash(fd, num);
	if (key < 0) return -1;
	list<Triple>& lst = table_[key];
	list<Triple>::const_iterator it;
	for (it = lst.begin(); it != lst.end(); it++) {
		if ((it->fd == fd) && (it->num == num))
			return it->slot;
	}
	return -1;
}


//
// insert - ��hash���в���һ��Ԫ��
// 
bool PFHashTable::insert(FILE* fd, Page num, int slot)
{
	int key = calcHash(fd, num);
	if (key < 0) return false;
	list<Triple>& lst = table_[key];
	list<Triple>::const_iterator it;
	// table�в����Ѿ�����������entry
	for (it = lst.begin(); it != lst.end(); it++) 
	{
		if ((it->fd == fd) && (it->num == num))
			return false;
	}
	lst.push_back(Triple(fd, num, slot));
	return true;
}

//
// remove - ��hash�����Ƴ���һ��Ԫ��
// 
bool PFHashTable::remove(FILE* fd, Page num)
{
	int key = calcHash(fd, num);
	if (key < 0) return false;
	list<Triple>& lst = table_[key];
	list<Triple>::const_iterator it;
	// table�в����Ѿ�����������entry
	for (it = lst.begin(); it != lst.end(); it++) {
		if ((it->fd == fd) && (it->num == num)) {
			lst.erase(it);
			return true;
		}
	}
	return false;
}