#ifndef PF_MANAGER_H
#define PF_MANAGER_H
#include "pf.h"
#include "pf_buffer.h"
#include "pf_filehandle.h"
//
// PFManager -	�ṩPF�ļ�����Ĺ��ܡ�
//
class PFManager {
public:
	PFManager() : buff_(PFBuffer::instance()) {};
	~PFManager() {};
	RC createFile(const char* pathname,int len);
	RC destroyFile(const char* pathname);
	RC openFile(const char* pathname, PFFilePtr& file);
	RC closeFile(PFFilePtr& file);
	
private:
	PFBuffer* buff_;
};

#endif /* PF_MANAGER_H */