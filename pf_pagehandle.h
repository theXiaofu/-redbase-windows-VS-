#ifndef PF_PAGE_HANDLE_H
#define PF_PAGE_HANDLE_H
#include "pf.h"
#include <memory>
using namespace std;

class PFFileHandle;
//
// PFPageHandle�Ƕ���ҳ���һ�ֳ���
//
class PFPageHandle  {
	friend class PFFileHandle;
public:
	PFPageHandle() : num_(-1), addr_(nullptr), dirty_(false) {}
	~PFPageHandle();
	PFPageHandle(const PFPageHandle& rhs);
	PFPageHandle& operator=(const PFPageHandle& page);

	//
	// rawPtr - ��ȡָ��ʵ�����ݵ���ָ��
	// 
	Ptr rawPtr() { return addr_; }
	Page page() { return num_; }
	void setOwner(shared_ptr<PFFileHandle> file) { pffile_ = file; }
	void setDirty();
	void dispose();		// �������ڵ�ҳ
private:
	bool dirty_;
	weak_ptr<PFFileHandle> pffile_;	// ���������б�Ҫ��
	Page num_;						// ҳ��ı��
	Ptr addr_;						// ָ��ʵ�ʵ�ҳ������
};

#endif /* PF_PAGE_HANDLE_H */