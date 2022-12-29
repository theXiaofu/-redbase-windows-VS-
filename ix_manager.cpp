

#include <windows.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <climits>
#include <string>
#include <sstream>
#include <cstdio>
#include "comparators.h"
#define PATH_MAX 50

IX_Manager::IX_Manager(PFManager& pfm) : pfm(pfm) 
{}

IX_Manager::~IX_Manager() 
{}

/////////////////////////////////////////////���ܽӿ�/�ⲿ�ӿ�///////////////////////////////////////////////////

//////////////////////////////��������////////////////////////////////////
/*
 �����ļ����������ţ����������ͺͳ���������һ������
 */
int IX_Manager::CreateIndex(const char* fileName, int indexNo,
    AttrType attrType, int attrLength) 
{

    // ����PF�Ķ���Ϊ��������Щ����ĺ�����������������Ҫ�ı�Ҫ��Ϣ
    PFFilePtr fh;
    PFPageHandle ph_header;
    PFPageHandle ph_root;
    // ������������������ҳ�棺һ���������ļ�ͷҳ
   // ��¼��������Ӧ���Ե������Ϣ��һ���ǿյĸ�����ҳ��
    Page headerpage;
    Page rootpage;
    struct IX_IndexHeader* header;
    struct IX_NodeHeader_L* rootheader;
    struct Node_Entry* entries;
    //����ÿ���ڵ��key�������ͼ���ÿ��bucket��key�����������������ڲ�����
    int numKeys_N = IX_IndexHandle::CalcNumKeysNode(attrLength);
    int numKeys_B = IX_IndexHandle::CalcNumKeysBucket(attrLength);

    if (fileName == NULL || indexNo < 0) // �����ļ������������Ƿ�Ϸ�
        return (IX_BADFILENAME);
    int rc = 0;
    if (!IsValidIndex(attrType, attrLength)) // ����isvalidindex���������Եĳ��Ⱥ������Ƿ�Ϸ�
        return (IX_BADINDEXSPEC);

    std::string indexname;
    if ((rc = GetIndexFileName(fileName, indexNo, indexname)))
        return (rc);
    //.c_str����Ϊ��c������û��string���ͣ��ʱ���ͨ��string�����
    //�ĳ�Ա����c_str()��string ����ת����c�е��ַ�����ʽ��������Ϊ�˺�c����
    if ((rc = pfm.createFile(indexname.c_str(),1<<20))) 
    {
        return (rc);
    }
    if ((rc = pfm.openFile(indexname.c_str(), fh)))
        return (rc);
    ph_header = PFAllocPage(fh);
    headerpage = ph_header.page();
    ph_root = PFAllocPage(fh);
    rootpage = ph_root.page();

    //��headerָ��ָ��ph_headerָ������ҳ�������
    //��rootheaderָ��ph_rootָ���ҳ�������
    //rc = ph_header.GetData((char*&)header)

    (char*&)header = ph_header.rawPtr();
    (char*&)rootheader = ph_root.rawPtr();
    // ��������ͷ�������Ϣ
    header->attr_type = attrType;
    header->attr_length = attrLength;
    header->maxKeys_N = numKeys_N;
    header->maxKeys_B = numKeys_B;
    header->entryOffset_N = sizeof(struct IX_NodeHeader_I);//node��ƫ����
    header->entryOffset_B = sizeof(struct IX_BucketHeader);//bucket����ڵ�ַ����Ϊ
    //bucket����Ӳ�̵Ķ�ȡλ�ã�����Ӧ�÷����ļ���ͷ��
    header->keysOffset_N = header->entryOffset_N + numKeys_N * sizeof(struct Node_Entry);
    //���Ե�ƫ����
    header->rootPage = rootpage;
    //���ø�����ҳ�����ز���
    rootheader->isLeafNode = true;
    rootheader->isEmpty = true;
    rootheader->num_keys = 0;
    rootheader->nextPage = NO_MORE_PAGES;//��Ϊ���½���������������һҳ����һҳ����û����Ϣ
    rootheader->prevPage = NO_MORE_PAGES;
    rootheader->firstSlotIndex = NO_MORE_SLOTS;
    rootheader->freeSlotIndex = 0;
    entries = (struct Node_Entry*)((char*)rootheader + header->entryOffset_N);
    //��ʼ��
    int i = 0;
    while (i < header->maxKeys_N) {
        entries[i].isValid = UNOCCUPIED;
        entries[i].page = NO_MORE_PAGES;
        entries[i].slot = NO_MORE_SLOTS;
        if (i == (header->maxKeys_N - 1))
            entries[i].nextSlot = NO_MORE_SLOTS;
        else
            entries[i].nextSlot = i + 1;
        ++i;
    }
    //printf("NODE CREATION: entries[0].nextSlot: %d \n", entries[0].nextSlot);

    // Mark both pages as dirty, and close the file
cleanup_and_exit:
    RC rc2;
    rc2 = fh.get()->markDirty(headerpage);
    rc2 = fh.get()->unpin(headerpage);
    rc2 = fh.get()->markDirty(rootpage);
    rc2 = fh.get()->unpin(rootpage);
    rc2 = pfm.closeFile(fh);
    return (rc);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////��������///////////////////////////////
/*
 * This function destroys a valid index given the file name and index number.
 */
RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
    int rc;
    if (fileName == NULL || indexNo < 0)
        return (IX_BADFILENAME);
    std::string indexname;
    rc = GetIndexFileName(fileName, indexNo, indexname);
        if (rc != 0)    return (rc);
        rc = pfm.destroyFile(indexname.c_str());
        if (rc != 0)    return (rc);
    return (0);
}
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////������////////////////////////////////////////

RC IX_Manager::OpenIndex(const char* fileName, int indexNo,
    IX_IndexHandle& indexHandle) {
    if (fileName == NULL || indexNo < 0) { // check for valid filename, and valid indexHandle
        return (IX_BADFILENAME);
    }
    if (indexHandle.isOpenHandle != false) {
        return (IX_INVALIDINDEXHANDLE);
    }
    RC rc = 0;
    PFPageHandle ph;
    Page firstpage;
    PFFilePtr fh;
    std::string indexname;
    char* pData;
    if ((rc = GetIndexFileName(fileName, indexNo, indexname)) ||
        (rc = pfm.openFile(indexname.c_str(), fh)))//����������ļ�
        return (rc);
    if ((rc = fh.get()->firstPage(ph))) 
    {
        //|| (ph.GetPageNum(firstpage)) || (ph.GetData(pData))
        firstpage = ph.page();
        pData = ph.rawPtr();
        //fh.UnpinPage(firstpage);
        fh.get()->unpin(firstpage);
        pfm.closeFile(fh);
        return (rc);
    }
    firstpage = ph.page();
    pData = ph.rawPtr();
    struct IX_IndexHeader* header = (struct IX_IndexHeader*)pData;//����һҳ������ͷȡ����

    rc = SetUpIH(indexHandle, fh, header);
    int rc2;
    if ((rc2 = fh.get()->unpin(firstpage)))
        return (rc2);

    if (rc != 0) 
    {
        pfm.closeFile(fh);
    }
    return (rc);
}
/////////////////////////////////////////////////////////////////////////////
///////////////////////////////////�ر�����//////////////////////////////////

RC IX_Manager::CloseIndex(IX_IndexHandle& indexHandle) {
    int rc = 0;
    PFPageHandle ph;
    Page page;
    char* pData;

    if (indexHandle.isOpenHandle == false) 
    { 
        //�ж����handle�Ƿ��Ǵ�״̬���������Ƿ�
        //�򿪣�û�򿪾ͷ��ر���
        return (IX_INVALIDINDEXHANDLE);
    }

    // �����ڵ�����ҳ��markdity����֤����д��Ӳ��
    Page root = indexHandle.header.rootPage;
    if ((rc = indexHandle.pfh.get()->markDirty(root)) || (rc = indexHandle.pfh.get()->unpin(root)))
        return (rc);

    // �������������ͷҳ���Ƿ�����޸ģ�����У�Ҳ���޸Ĺ���ͷҳ����и���
    if (indexHandle.header_modified == true)
    {
        if ((rc = indexHandle.pfh.get()->firstPage(ph)))
        {
            return (rc);
        }
        page = ph.page();
        pData = ph.rawPtr();
        if (rc != 0) {
            RC rc2;
            if ((rc2 = indexHandle.pfh.get()->unpin(page)))
                return (rc2);
            return (rc);
        }
        memcpy(pData, &indexHandle.header, sizeof(struct IX_IndexHeader));
        if ((rc = indexHandle.pfh.get()->markDirty(page)) || (rc = indexHandle.pfh.get()->unpin(page)))
            return (rc);
    }

    // �ر��ļ�
    if ((rc = pfm.closeFile(indexHandle.pfh)))
        return (rc);

    if (indexHandle.isOpenHandle == false)
        return (IX_INVALIDINDEXHANDLE);
    indexHandle.isOpenHandle = false;
    return (rc);
}

////////////////////////////////////////////////////////////////////////////////

















/*
 * This function checks that the parameters passed in for attrType and attrLength
 * make it a valid Index. Return true if so.
 */
bool IX_Manager::IsValidIndex(AttrType attrType, int attrLength) 
{
    if (attrType == INT0 && attrLength == 4)
        return true;
    else if (attrType == FLOAT0 && attrLength == 4)
        return true;
    else if (attrType == STRING0 && attrLength > 0 && attrLength <= MAXSTRINGLEN)
        return true;
    else
        return false;
}

/*
 * This function is given a fileName and index number, and returns the name of the
 * index file to be created as a string in indexname.
 */
RC IX_Manager::GetIndexFileName(const char* fileName, int indexNo, std::string& indexname) {

    std::stringstream convert;
    convert << indexNo;
    std::string idx_num = convert.str();
    indexname = std::string(fileName);
    indexname.append(".");
    indexname.append(idx_num);
    if (indexname.size() > PATH_MAX || idx_num.size() > 10)
        return (IX_BADINDEXNAME);
    return (0);
}




RC IX_Manager::SetUpIH(IX_IndexHandle& ih, PFFilePtr& fh, struct IX_IndexHeader* header) {
    int rc = 0;
    memcpy(&ih.header, header, sizeof(struct IX_IndexHeader));
    if (!IsValidIndex(ih.header.attr_type, ih.header.attr_length)) 
        return (IX_INVALIDINDEXFILE);
    if (!ih.isValidIndexHeader()) 
        return (rc);


    rc = fh.get()->getPage(header->rootPage, ih.rootPH);
        if (rc != 0) return (rc);




    if (ih.header.attr_type == INT0) {
        ih.comparator = compare_int;
        ih.printer = print_int;
    }
    else if (ih.header.attr_type == FLOAT0) {
        ih.comparator = compare_float;
        ih.printer = print_float;
    }
    else 
    {
        ih.comparator = compare_string;
        ih.printer = print_string;
    }

    ih.header_modified = false;
    ih.pfh = fh;
    ih.isOpenHandle = true;
    return (rc);
}

RC IX_Manager::CleanUpIH(IX_IndexHandle& indexHandle) 
{
    if (indexHandle.isOpenHandle == false)
        return (IX_INVALIDINDEXHANDLE);
    indexHandle.isOpenHandle = false;
    return (0);
}

