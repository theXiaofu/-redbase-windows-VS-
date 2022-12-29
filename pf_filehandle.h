#ifndef PF_FILEHANDLE_H
#define PF_FILEHANDLE_H
#include "pf.h"
#include "pf_buffer.h"
#include "pf_pagehandle.h"
#include "noncopyable.h"
#include <memory>
using namespace std;

//
// PFFileHandle - ���ļ���һ�ֳ���,���ļ�ʱ��������Ķ���,Ҫ�ر��ļ�ʱ,���ٸö���
// 
using PFFilePtr = shared_ptr<PFFileHandle>;
class PFFileHandle : noncopyable 
{
	friend class PFManager;
public:
	PFFileHandle()
		:opened_(false)
		, buff_(PFBuffer::instance()),changed_(false),fd_(NULL)
	{}
	~PFFileHandle()
	{
		
		flush();
		clearFilePages();
	};
public:
	RC firstPage(PFPageHandle& page) const;
	RC nextPage(Page curr, PFPageHandle& page) const;
	RC getPage(Page num, PFPageHandle& page) const;
	RC lastPage(PFPageHandle& page) const;
	RC prevPage(Page curr, PFPageHandle& page) const;

	RC allocPage(PFPageHandle& page);
	RC disposePage(Page num);
	RC markDirty(Page num) const;
	RC unpin(Page num) const;
	RC pin(Page num);
	RC forcePages(Page num = ALL_PAGES);
private:
	void clearFilePages();
	RC flush();
private:
	PFBuffer* buff_;				// PFBuffer��������ڹ�����
	PFFileHdr hdr_;					// ���ڼ�¼�ļ�ͷ������Ϣ
	bool opened_;					// �ļ��Ƿ��Ѿ���
	bool changed_;					// �ļ�ͷ���Ƿ��Ѿ��ı���
	FILE* fd_;						// �ļ�������
};
PFPageHandle PFAllocPage(PFFilePtr& file);
PFPageHandle PFGetPage(PFFilePtr& file, Page num);




#endif /* PF_FILEHANDLE_H */