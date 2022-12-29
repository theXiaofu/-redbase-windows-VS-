#ifndef PF_BUFFER_H
#define PF_BUFFER_H

#include <string.h>
#include "pf_internal.h"
#include "pf_hashtable.h"
#include "pf_error.h"
using namespace std;


//
// PFBuffer - ���ڻ����ļ���ҳ��
//
class PFBuffer {
public:
	const static int capacity = 40;//�����С
	const static int pagesize = PF_PAGE_SIZE + sizeof(PFPageHdr);//ҳ��С
private:
	struct BufferNode {
		bool dirty;
		uint count;
		FILE* fd;   //��߿���Ҫ�Ķ�
		Page num;
		bool red;//����ʱ���㷨
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
	
	//int first_;//����ע�͵�ԭ����û���õ���Ϊfirst��last_��˹̹����ѧgithubԴ����Ϊ�˲���LRU
	//int last_;//�㷨��ʹ�õ���Ϊ������û�в������з�ʽֻ�Ǽ򵥵�����ɨ�����н���ע��
	//int free_;
};

#endif