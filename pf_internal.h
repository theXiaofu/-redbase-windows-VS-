//
// File:		pf_internal.h
// Description:	Declarations internal to the paged file component
//

#ifndef PF_INTERNAL_H
#define PF_INTERNAL_H

#include <cstdlib>//һ�ֺ궨�庯������ֹ�����йأ���ʱ����̫���
#include <cstring>
#include "pf.h"

// Constants and defines
#define CREATION_MASK		0600		// r/w privileges to owner only
#define PF_PAGE_LIST_END	-1			// end of list of free_ pages
#define PF_PAGE_USED		-2			// Page is being used


//
// PageHdr: Header structure for pages
//
struct PFPageHdr 
{
	int free;		// free���������¼���ȡֵ:
					// - ��һ������ҳ�ı��,��ʱ��ҳ��Ҳ�ǿ��е�
					// - PF_PAGE_LIST_END => ҳ�������һ������ҳ��
					// - PF_PAGE_USED => ҳ�沢���ǿ��е�
};

// Justify the file header to the length of one Page
//���ļ�header����Ϊһ��ҳ�ĳ���
const int PF_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(PFPageHdr);

#endif /* PF_INTERNAL_H */