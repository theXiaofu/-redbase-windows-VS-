#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <windows.h>
#include <sys/types.h>
#include "rm_manager.h"
#include "pf_pagehandle.h"
#include "rm_filehandle.h"

extern PFManager pfManager;

RC RMManager::createFile(const char* pathname, int len)
{
	if (len == 0) return RM_BADRECSIZE;
	if (pathname == NULL) return -1;
	RC rc;
	if (rc = pfManager.createFile(pathname, len))
	{
		return (rc);
	}
	//if (rcdlen >= (PF_PAGE_SIZE - sizeof(RMPageHdr))) return RM_SIZETOOBIG;
	//pfManager.createFile(pathname,);
	PFFilePtr file;
	pfManager.openFile(pathname, file);
	PFPageHandle page = PFAllocPage(file);
	Ptr addr = page.rawPtr();
	

	// 文件的第一块是头部信息
	RMFileHdr* hdr = (RMFileHdr*)addr;
	hdr->free = PAGE_LIST_END;//这里表示空闲页链为空
							  // 第0页仅仅用作RM文件头,不做别的使用.
	hdr->size = 0;				// 已经分配了的页的数目
	page.setDirty();
	pfManager.closeFile(file);
	return 0;
}

RC RMManager::destroyFile(const char* pathname)
{
	return pfManager.destroyFile(pathname);
}

//
// openFile - 打开某个文件
// 
RC RMManager::openFile(const char* pathname, RMFilePtr& rmFile)
{
	PFFilePtr pfFile;
	RC errval;
	errval = pfManager.openFile(pathname, pfFile);
	if (errval != 0) return errval;
	rmFile = make_shared<RMFileHandle>(pfFile);
	return 0;
}

//
// closeFile 将文件关闭
// 
RC RMManager::closeFile(RMFilePtr& file)
{
	if (file == nullptr) return RM_FNOTOPEN;
	PFFilePtr pffile = file->pffile_;
	file.reset();
	pfManager.closeFile(pffile);
	return 0;
}