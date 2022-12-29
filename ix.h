

#ifndef IX_H
#define IX_H


#include "redbase.h" 
#include "rm_rid.h"  
#include "pf.h"
#include <string>
#include <stdlib.h>
#include <cstring>
#include "pf_filehandle.h"
#include "pf_pagehandle.h"
#include "pf_manager.h"
//////////////////////////////////////////////////////����ṹ��/////////////////////////////////////
// ������������ҳ��ͷ��������¼�ýڵ�Ļ�����Ϣ
struct IX_IndexHeader 
{
    AttrType attr_type; // �������Ⱥ�����
    int attr_length;

    int entryOffset_N;  // �ڵ���ڵ�ƫ����
    int entryOffset_B;  // bucket�����ƫ����

    int keysOffset_N;   // ���Ե�ƫ����

    int maxKeys_N;      // ���������bucket�ͽڵ�
    int maxKeys_B;

    Page rootPage;   //�ҵ����ڵ����ڵ�page
};

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////����IndexHandle��//////////////////////////////////////////
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
    static const int BEGINNING_OF_SLOTS = -2; // �ೣ��
    static const int END_OF_SLOTS = -3;
    static const char UNOCCUPIED = 'u';
    static const char OCCUPIED_NEW = 'n';
    static const char OCCUPIED_DUP = 'r';
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // �ڴ򿪵������ļ��в���һ���µ�������ʵ�ַ�ʽ�������ڴ򿪵��ļ�ҳ�������һ����¼,����ֵΪpdata����¼Ϊrid
    RC InsertEntry(void* pData, const RID& rid);

    // D�ڴ򿪵�����������ɾ������ֵΪpdata����¼Ϊrid��һ������
    RC DeleteEntry(void* pData, const RID& rid);

    // ��handle���������ҳȫ�����ڴ�д��Ӳ��
    RC ForcePages();
    RC PrintIndex();
private:
    // �ڲ��ӿں͸����ӿ�
    //�ں�������õ�ʱ����ϸ����
    RC CreateNewNode(PFPageHandle& ph, Page& page, char*& nData, bool isLeaf);
    RC CreateNewBucket(Page& page);
    RC InsertIntoNonFullNode(struct IX_NodeHeader* nHeader, Page thisNodeNum, void* pData, const RID& rid);
    RC InsertIntoBucket(Page page, const RID& rid);
    RC SplitNode(struct IX_NodeHeader* pHeader, struct IX_NodeHeader* oldHeader, Page oldPage, int index,
        int& newKeyIndex, Page& newPageNum);
    RC FindPrevIndex(struct IX_NodeHeader* nHeader, int thisIndex, int& prevIndex);
    RC FindNodeInsertIndex(struct IX_NodeHeader* nHeader,
        void* pData, int& index, bool& isDup);
    RC DeleteFromNode(struct IX_NodeHeader* nHeader, void* pData, const RID& rid, bool& toDelete);
    RC DeleteFromBucket(struct IX_BucketHeader* bHeader, const RID& rid,
        bool& deletePage, RID& lastRID, Page& nextPage);
    RC DeleteFromLeaf(struct IX_NodeHeader_L* nHeader, void* pData, const RID& rid, bool& toDelete);
    bool isValidIndexHeader() const;
    static int CalcNumKeysNode(int attrLength);
    static int CalcNumKeysBucket(int attrLength);
    RC GetFirstLeafPage(PFPageHandle& leafPH, Page& leafPage);
    RC FindRecordPage(PFPageHandle& leafPH, Page& leafPage, void* key);
    bool isOpenHandle;
    PFFilePtr pfh;
    bool header_modified;
    PFPageHandle rootPH; //����һ��PFPageHandle�Ķ����������PFPageHandle����ķ�����
    struct IX_IndexHeader header;
    int (*comparator) (void*, void*, int);
    bool (*printer) (void*, int);

};
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////����manager��///////////////////////////////////////
//
// IX_Manager: provides IX index file management
//
class IX_Manager
{
    static const char UNOCCUPIED = 'u';
public:
    IX_Manager(PFManager& pfm);
    ~IX_Manager();


    // ��һ������
    RC OpenIndex(const char* fileName, int indexNo,
        IX_IndexHandle& indexHandle);

    //�ر�һ������
    RC CloseIndex(IX_IndexHandle& indexHandle);


    //�����µ�����
    RC CreateIndex(const char* fileName, int indexNo,
        AttrType attrType, int attrLength);

    // ����һ������
    RC DestroyIndex(const char* fileName, int indexNo);



private:

    //ͨ���ļ����������������������
    RC GetIndexFileName(const char* fileName, int indexNo, std::string& indexname);
    //�������������������ڲ���ز���
    RC SetUpIH(IX_IndexHandle& ih, PFFilePtr& fh, struct IX_IndexHeader* header);
    //�жϸ��������ǿ��õ�
    bool IsValidIndex(AttrType attrType, int attrLength);
    //�ر��ļ������������ڲ�����
    RC CleanUpIH(IX_IndexHandle& indexHandle);

    PFManager& pfm; // ����һ��PF����������PF_manager�ķ�����

};

//////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////����IndexScan��/////////////////////////////////////////


class IX_IndexScan {
    static const char UNOCCUPIED = 'u';    // class constants to check whether
    static const char OCCUPIED_NEW = 'n';  // a slot in a node is valid
    static const char OCCUPIED_DUP = 'r';
public:
    IX_IndexScan();
      ~IX_IndexScan();

      //��һ��
      RC OpenScan(const IX_IndexHandle& indexHandle,
          Operator compOp,
          const void* value,
          ClientHint  pinHint = NO_HINT);


      // Get the next matching entry return IX_EOF if no more matching
      // entries.
      RC GetNextEntry(RID& rid);

      // Close index scan
      RC CloseScan();
      RC rewind();
private:
    RC BeginScan(PFPageHandle& leafPH, Page& pageNum);
    // Sets up the scan private variables to the first entry within the given leaf
    RC GetFirstEntryInLeaf(PFPageHandle& leafPH);
    // Sets up the scan private variables to the appropriate entry within the given leaf
    RC GetAppropriateEntryInLeaf(PFPageHandle& leafPH);
    // Sets up the scan private variables to the first entry within this bucket
    RC GetFirstBucketEntry(Page nextBucket, PFPageHandle& bucketPH);
    // Sets up the scan private variables to the next entry in the index
    RC FindNextValue();

    // Sets the RID
    RC SetRID(bool setCurrent);


    bool openScan;              // ��ʾscan�Ƿ���
    IX_IndexHandle* indexHandle;// Pointer to the indexHandle that modifies the
                                // file that the scan will try to traverse

    // The comparison to determine whether a record satisfies given scan conditions
    //�Ƚϼ�¼�Ƿ���������������
    bool (*comparator) (void*, void*, AttrType, int);
    int attrLength;     // Comparison type, and information about the value
    void* value;        // to compare to�Ƚϵ�ֵ��������Ϣ��
    AttrType attrType;
    Operator compOp;

    bool scanEnded;     // Indicators for whether the scan has started or 
    bool scanStarted;   // ended��ʾɨ���Ƿ�ʼ���߽���

    PFPageHandle  currLeafPH;   // Currently pinned Leaf and Bucket PageHandles
    PFPageHandle currBucketPH; // that the scan is accessingɨ�����ڷ��ʵĵ�ǰ��Ҷ�Ӻ�Ͱ
                               //��PageHandles

    char* currKey;              // the keys of the current record, and the following
    char* nextKey;              // two records after that
    char* nextNextKey;          //��ǰ��¼�ļ�ֵ�����������¼�ļ�ֵ
    struct IX_NodeHeader_L* leafHeader;     // the scan's current leaf and bucket header
    struct IX_BucketHeader* bucketHeader;   // and entry and key pointers
    struct Node_Entry* leafEntries;         //ɨ��ĵ�ǰ��Ҷ�Ӻ�Ͱ��ͷ�����
    struct Bucket_Entry* bucketEntries;
    char* leafKeys;
    int leafSlot;               // the current leaf and bucket slots of the scan
    int bucketSlot;
    Page currLeafNum;        // the current and next bucket slots of the scan
    Page currBucketNum;
    Page nextBucketNum;

    RID currRID;    // the current RID and the next RID in the scan
    RID nextRID;

    bool hasBucketPinned; // whether the scan has pinned a bucket or a leaf page
    bool hasLeafPinned;
    bool initializedValue; // Whether value variable has been initialized (malloced)
    bool endOfIndexReached; // Whether the end of the scan has been reached


    bool foundFirstValue;
    bool foundLastValue;
    bool useFirstLeaf;
};




//
// ��ӡ������Ϣ
//
void IX_PrintError(RC rc);

#define IX_BADINDEXSPEC         (START_IX_WARN + 0) // Bad Specification for Index File
#define IX_BADINDEXNAME         (START_IX_WARN + 1) // Bad index name
#define IX_INVALIDINDEXHANDLE   (START_IX_WARN + 2) // FileHandle used is invalid
#define IX_INVALIDINDEXFILE     (START_IX_WARN + 3) // Bad index file
#define IX_NODEFULL             (START_IX_WARN + 4) // A node in the file is full
#define IX_BADFILENAME          (START_IX_WARN + 5) // Bad file name
#define IX_INVALIDBUCKET        (START_IX_WARN + 6) // Bucket trying to access is invalid
#define IX_DUPLICATEENTRY       (START_IX_WARN + 7) // Trying to enter a duplicate entry
#define IX_INVALIDSCAN          (START_IX_WARN + 8) // Invalid IX_Indexscsan
#define IX_INVALIDENTRY         (START_IX_WARN + 9) // Entry not in the index
#define IX_EOF                  (START_IX_WARN + 10)// End of index file
#define IX_LASTWARN             IX_EOF

#define IX_ERROR                (START_IX_ERR - 0) // error
#define IX_LASTERROR            IX_ERROR

#endif
