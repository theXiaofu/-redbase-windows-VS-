#ifndef PF_BUFFER_H
#define PF_BUFFER_H

#include <string.h>
#include "pf_internal.h"
#include "pf_hashtable.h"
#include "pf_error.h"
using namespace std;


//
// PFBuffer - 用于缓存文件的页面
//
class PFBuffer {
public:
	const static int capacity = 40;//缓存大小
	const static int pagesize = PF_PAGE_SIZE + sizeof(PFPageHdr);//页大小
private:
	struct BufferNode {
		bool dirty;
		uint count;
		FILE* fd;   //后边可能要改动
		Page num;
		bool red;//采用时钟算法
		char buffer[pagesize];
		BufferNode()
			: dirty(false), count(0)
			, fd(NULL), num(-1),red(false)
		{
			// bzero(buffer, sizeof(buffer));
			memset(buffer, 0, sizeof(buffer));
		}
	};
private:
	PFBuffer() : table_(capacity / 2) 
	{
		
	}
public:
	static PFBuffer* instance()
	{
		if (instance_ == nullptr) 
		{
			instance_ = new PFBuffer();
		}
		return instance_;
	}

	static void destroyBuffer()
	{
		if (instance_ != nullptr) {
			delete instance_;
			instance_ = nullptr;
		}
	}

public:
	RC getPage(FILE* fd, Page num, Ptr& addr);
	RC allocPage(FILE* fd, Page num, Ptr& addr);
	RC markDirty(FILE* fd, Page num);
	RC unpin(FILE* fd, Page num);
	RC pin(FILE* fd, Page num);
	RC forcePages(FILE* fd, Page num);
	void clearFilePages(FILE* fd);
	RC flush(FILE* fd);
private:
	int searchAvaliableNode();
	RC readPage(FILE* fd, Page num, Ptr dst);
	RC writeBack(FILE* fd, Page num, Ptr src);
	
private:
	static PFBuffer* instance_;
	BufferNode nodes_[capacity];
	PFHashTable table_;
	
	//int first_;//将其注释的原因是没有用到因为first和last_在斯坦福大学github源码上为了采用LRU
	//int last_;//算法而使用的因为本代码没有采用这中方式只是简单的线性扫描所有将其注释
	//int free_;
};

#endif