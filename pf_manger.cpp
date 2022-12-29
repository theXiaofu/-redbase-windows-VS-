#include <cstdio>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pf_internal.h"
#include "pf_buffer.h"
#include "pf_filehandle.h"
#include "util.h"
#include "pf_manager.h"
#include "pf_error.h"

using namespace RedBase;


RC PFManager::createFile(const char* pathname,int len)
{
	FILE* fd = NULL;
	int n;
	// O_CREAT ����򿪵��ļ������ڵĻ������Զ��������ļ�
	// O_EXCL ���O_CREATҲ������, ��ָ���ȥ����ļ��Ƿ����. �ļ����������������ļ�, 
	// ���򽫵��´��ļ�����.
	// O_WRONLY ��ֻд�ķ�ʽ���ļ�
	//fd = fopen(pathname, "W +" );
	Creat(pathname, len);
	//printf("1<<20:%d\n", 1 << 20);
	fd = Open(pathname, "rb+");
	char hdrBuf[PF_FILE_HDR_SIZE];
	memset(hdrBuf, 0, PF_FILE_HDR_SIZE);


	PFFileHdr* hdr_ = (PFFileHdr*)hdrBuf;
	hdr_->free = PF_PAGE_LIST_END;
	hdr_->size = 0;

	// ��ͷ��д�뵽�ļ���
	n = Write(fd, hdrBuf, PF_FILE_HDR_SIZE);
	//printf("%d  %d 2111\n", n, PF_FILE_HDR_SIZE);
	if (n != 1) 
	{
		Close(fd);
		//Unlink(pathname); // ɾ���ļ�
		remove(pathname);
		if (n < 0) return PF_UNIX;
		else return PF_HDRWRITE;
	}
	Close(fd);
	return 0; // һ�ж�OK
}

//
// destroyFile - ɾ�����ļ�
// 
RC PFManager::destroyFile(const char* pathname)
{
	//Unlink(pathname);
	
	return remove(pathname);
}

//
// openFile - ��ĳ���ļ�,�����ļ���ͷ����Ϣ����,д�뵽PFFileHandle���ʵ����
// 
RC PFManager::openFile(const char* pathname, PFFilePtr& file)
{
	RC rc;
	file = make_shared<PFFileHandle>();
	file->fd_ = Open(pathname, "rb+");
	//printf("��1");
	int n = Read(file->fd_, (Ptr)&file->hdr_, sizeof(PFFileHdr));
	if (n != 1) 
	{
		//printf("n:%d", n);
		Close(file->fd_);
		return PF_HDRREAD;
	}
	//printf("free:%d\n", file->hdr_.free);
	file->changed_ = false;
	file->opened_ = true;
	return 0;
}

//
// closeFile - �رյ��ļ�
// 
RC PFManager::closeFile(PFFilePtr& file)
{
	// ��buffer�еĶ���ˢ�µ�������
	FILE* fd = file->fd_;
	file.reset();
	// �ر��ļ�
	Close(fd);
	return 0;
}


