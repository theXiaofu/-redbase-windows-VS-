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
// PFHashTable - 一个非常简易的hash表实现,你甚至可以说,简单到有些不可思议,
// 这个table记录了一些什么呢?这个玩意其实就是一个map,记录了fd指代的这个文件的第num个页面
// 究竟在那个slot里面.
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
	vector<list<Triple>> table_;//哈希表里边存储着入口
};

#endif /* PF_HASHTABLE_H */