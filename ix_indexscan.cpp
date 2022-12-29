

#include <windows.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <cstdio>


bool iequal(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 == *(float*)value2);
    case INT0: return (*(int*)value1 == *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) == 0);
    }
}

bool iless_than(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 < *(float*)value2);
    case INT0: /*printf("%d %d\n", *(int*)value1, *(int*)value2);*/return (*(int*)value1 < *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) < 0);
    }
}

bool igreater_than(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 > *(float*)value2);
    case INT0: /*printf("%d %d\n", *(int*)value1, *(int*)value2);*/ return (*(int*)value1 > *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) > 0);
    }
}

bool iless_than_or_eq_to(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 <= *(float*)value2);
    case INT0: return (*(int*)value1 <= *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) <= 0);
    }
}

bool igreater_than_or_eq_to(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 >= *(float*)value2);
    case INT0: /*printf("%d %d\n", *(int*)value1, *(int*)value2);*/ return (*(int*)value1 >= *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) >= 0);
    }
}

bool inot_equal(void* value1, void* value2, AttrType attrtype, int attrLength) {
    switch (attrtype) {
    case FLOAT0: return (*(float*)value1 != *(float*)value2);
    case INT0: return (*(int*)value1 != *(int*)value2);
    default:
        return (strncmp((char*)value1, (char*)value2, attrLength) != 0);
    }
}
/////////////////////////////////////////功能接口///////////////////////////////
///////////////////////////////////////打开搜索////////////////////////

int IX_IndexScan::OpenScan(const IX_IndexHandle& indexHandle,
    Operator compOp,
    const void* value,
    ClientHint  pinHint) 
{
    int rc = 0;
    this->value = NULL;
    useFirstLeaf = true;
    this->compOp = compOp; // 将compOP传入
    this->attrType = (indexHandle.header).attr_type;
    if (openScan == true || compOp == NE_OP) //确保文件openscan现在是关闭状态以及compOp是可用的比较符
        return (IX_INVALIDSCAN);

    if (indexHandle.isValidIndexHeader()) // 确保indexhandle是可用的
        this->indexHandle = const_cast<IX_IndexHandle*>(&indexHandle);
    else
        return (IX_INVALIDSCAN);


    switch (compOp) {
    case EQ_OP: comparator = &iequal; useFirstLeaf = false; break;
    case LT_OP: comparator = &iless_than; break;
    case GT_OP: comparator = &igreater_than; useFirstLeaf = false; break;
    case LE_OP: comparator = &iless_than_or_eq_to; break;
    case GE_OP: comparator = &igreater_than_or_eq_to; useFirstLeaf = false; break;
    case NO_OP: comparator = NULL; break;
    default: return (IX_INVALIDSCAN);
    }

    // 将属性长度和类型传入
    attrLength = ((this->indexHandle)->header).attr_length;
    if (compOp != NO_OP) 
    {
        this->value = (void*)malloc(attrLength); // 按照属性的长度来分配内存
        memcpy(this->value, value, attrLength);
        initializedValue = true;
    }

    openScan = true; // 
    scanEnded = false;
    hasLeafPinned = false;
    scanStarted = false;
    endOfIndexReached = false;
    foundFirstValue = false;
    foundLastValue = false;

    return (rc);
}

///////////////////////////////////////////////////////////////////////
/////////////////////////////////////获取符合条件的下一条记录//////////////////////

RC IX_IndexScan::GetNextEntry(RID& rid) 
{
    RC rc = 0;
    if (scanEnded == true && openScan == true)
        return (IX_EOF);
    if (foundLastValue == true)
        return (IX_EOF);

    if (scanEnded == true || openScan == false)
        return (IX_INVALIDSCAN);

    bool notFound = true; // 标识是否已经找到了符合条件的记录
    while (notFound) 
    {

        if (scanEnded == false && openScan == true && scanStarted == false) {
            if ((rc = BeginScan(currLeafPH, currLeafNum)))//curr这俩是PFpagehandle对象
                return (rc);
            currKey = nextNextKey; // 存储目前的key 
            scanStarted = true; // 设定搜索开始的标识符
            SetRID(true);       // Set the current RID
            // 获取下一个值，如果已经到达末尾，则将扫描完毕的标识符设为true
            if ((IX_EOF == FindNextValue()))
                endOfIndexReached = true;
        }
        else 
        {
            currKey = nextKey;
            currRID = nextRID;
        }
        SetRID(false); // 将缓冲区中的元素设置为扫描的当前状态
        nextKey = nextNextKey;

        if ((IX_EOF == FindNextValue())) 
        {
            endOfIndexReached = true;
        }

        Page thisRIDPage;
        thisRIDPage = currRID.page();
        if (thisRIDPage == -1) 
        {
            scanEnded = true;
            return (IX_EOF);
        }


        if (compOp == NO_OP)
        {//如果比较符没有，则说明任何记录都可以，随便返回一个
            rid = currRID;
            notFound = false;
            foundFirstValue = true;
        }
        else if ((comparator((void*)currKey, value, attrType, attrLength))) 
        {
            rid = currRID;
            notFound = false;
            foundFirstValue = true;
        }
        else if (foundFirstValue == true) 
        {
            foundLastValue = true;
            return (IX_EOF);
        }

    }
    Slot thisRIDpage;
    //currRID.GetSlotNum(thisRIDpage);
    thisRIDpage = currRID.slot();
    return (rc);
}

///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////关闭搜索////////////////////////////////////////
RC IX_IndexScan::CloseScan() {
    RC rc = 0;
    if (openScan == false)
        return (IX_INVALIDSCAN);
    if (initializedValue == true) {
        free(value);
        initializedValue = false;
        if (scanEnded == false && hasBucketPinned == true)
            indexHandle->pfh.get()->unpin(currBucketNum);
        if (scanEnded == false && hasLeafPinned == true && (currLeafNum != (indexHandle->header).rootPage))
            indexHandle->pfh.get()->unpin(currLeafNum);

    }
    openScan = false;
    scanStarted = false;

    return (rc);
}

//////////////////////////////////////////////////////////////////////////////////////



RC IX_IndexScan::BeginScan(PFPageHandle& leafPH, Page& pageNum) {
    RC rc = 0;
    if (useFirstLeaf) 
    {
        if ((rc = indexHandle->GetFirstLeafPage(leafPH, pageNum)))
            return (rc);
        if ((rc = GetFirstEntryInLeaf(currLeafPH))) 
        {
            if (rc == IX_EOF) 
            {
                scanEnded = true;//搜索到了末尾
            }
            return (rc);
        }
    }
    else {
        if ((rc = indexHandle->FindRecordPage(leafPH, pageNum, value)))
            return (rc);
        if ((rc = GetAppropriateEntryInLeaf(currLeafPH))) {
            if (rc == IX_EOF) {
                scanEnded = true;
            }
            return (rc);
        }
    }
    return (rc);
}




///////////////////////////////////



IX_IndexScan::IX_IndexScan() {
    openScan = false;  // Initialize all values
    value = NULL;
    initializedValue = false;
    hasBucketPinned = false;
    hasLeafPinned = false;
    scanEnded = true;
    scanStarted = false;
    endOfIndexReached = true;
    attrLength = 0;
    attrType = INT0;

    foundFirstValue = false;
    foundLastValue = false;
    useFirstLeaf = false;
}

IX_IndexScan::~IX_IndexScan() 
{
    if (scanEnded == false && hasBucketPinned == true)
        indexHandle->pfh.get()->unpin(currBucketNum);
    if (scanEnded == false && hasLeafPinned == true && (currLeafNum != (indexHandle->header).rootPage))
        indexHandle->pfh.get()->unpin(currLeafNum);
    if (initializedValue == true) 
    {
        free(value);
        initializedValue = false;
    }

}

RC IX_IndexScan::rewind()
{
    useFirstLeaf = false;
    return 0;
}


/*
 * This function returns the next RID that meets the requirements of the scan
 */


 /*
  * This sets one of the private variable RIDs. If setCurrent is true, it sets
  * currRID. If setCurrent is false, it sets nextRID.
  */
RC IX_IndexScan::SetRID(bool setCurrent) 
{
    if (endOfIndexReached && setCurrent == false) 
    {
        RID rid1(-1, -1); // If we have reached the end of the scan, set the nextRID
        nextRID = rid1;   // to an invalid value.
        return (0);
    }

    if (setCurrent) 
    {
        if (hasBucketPinned) 
        { // if bucket is pinned, use bucket page/slot to set RID
            RID rid1(bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
            currRID = rid1;
        }
        else if (hasLeafPinned) 
        { // otherwise, use the leaf value to set page/slot of RID
            RID rid1(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
            currRID = rid1;
        }
    }
    else {
        if (hasBucketPinned) 
        {
            RID rid1(bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
            nextRID = rid1;
        }
        else if (hasLeafPinned) 
        {
            RID rid1(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
            nextRID = rid1;
        }
    }

    return (0);
}

/*
 * This function adavances the state of the search by one element. It updates all the
 * private variables assocaited with this scan.
 * 此函数将搜索状态提前一个元素。它更新与此扫描关联的所有私有变量。
 */
RC IX_IndexScan::FindNextValue() 
{
    RC rc = 0;
    if (hasBucketPinned) 
    { // If a bucket is pinned, then search in this bucket
        int prevSlot = bucketSlot;
        bucketSlot = bucketEntries[prevSlot].nextSlot; // update the slot index
        if (bucketSlot != NO_MORE_SLOTS) {
            return (0); // found next bucket slot, so stop searching
        }
        // otherwise, unpin this bucket
        Page nextBucket = bucketHeader->nextBucket;
        if ((rc = (indexHandle->pfh).get()->unpin(currBucketNum)))
            return (rc);
        hasBucketPinned = false;

        if (nextBucket != NO_MORE_PAGES) { // If this is a valid bucket, open it up, and get
                                         // the first entry
            if ((rc = GetFirstBucketEntry(nextBucket, currBucketPH)))
                return (rc);
            currBucketNum = nextBucket;
            return (0);
        }
    }
    // otherwise, deal with leaf level. 
    int prevLeafSlot = leafSlot;
    leafSlot = leafEntries[prevLeafSlot].nextSlot; // update to the next leaf slot

    // If the leaf slot containts duplicate entries, open up the bucket associated
    // with it, and update the key.
    if (leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_DUP) {
        nextNextKey = leafKeys + leafSlot * attrLength;
        currBucketNum = leafEntries[leafSlot].page;

        if ((rc = GetFirstBucketEntry(currBucketNum, currBucketPH)))
            return (rc);
        return (0);
    }
    // Otherwise, stay update the key.
    if (leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_NEW) {
        nextNextKey = leafKeys + leafSlot * attrLength;
        return (0);
    }

    // otherwise, get the next page
    Page nextLeafPage = leafHeader->nextPage;

    // if it's not the root page, unpin it:
    if ((currLeafNum != (indexHandle->header).rootPage)) {
        if ((rc = (indexHandle->pfh).get()->unpin(currLeafNum))) {
            return (rc);
        }
    }
    hasLeafPinned = false;

    // If the next page is a valid page, then retrieve the first entry in this leaf page
    if (nextLeafPage != NO_MORE_PAGES) 
    {
        currLeafNum = nextLeafPage;
        if ((rc = (indexHandle->pfh).get()->getPage(currLeafNum, currLeafPH)))
            return (rc);
        if ((rc = GetFirstEntryInLeaf(currLeafPH)))
            return (rc);
        return (0);
    }

    return (IX_EOF); // Otherwise, no more elements

}


/*
 * This function retrieves the first entry in an open leafPH.
 */
RC IX_IndexScan::GetFirstEntryInLeaf(PFPageHandle& leafPH) {
    RC rc = 0;
    hasLeafPinned = true;
    (char*&)leafHeader = leafPH.rawPtr();

    if (leafHeader->num_keys == 0) // no keys in the leaf... return IX_EOF
        return (IX_EOF);

    leafEntries = (struct Node_Entry*)((char*)leafHeader + (indexHandle->header).entryOffset_N);
    leafKeys = (char*)leafHeader + (indexHandle->header).keysOffset_N;

    leafSlot = leafHeader->firstSlotIndex;
    if ((leafSlot != NO_MORE_SLOTS)) {
        nextNextKey = leafKeys + attrLength * leafSlot; // set the key to the first value
    }
    else
        return (IX_INVALIDSCAN);
    if (leafEntries[leafSlot].isValid == OCCUPIED_DUP) { // if it's a duplciate value, go into the bucket
        currBucketNum = leafEntries[leafSlot].page;      // to retrieve the first entry
        if ((rc = GetFirstBucketEntry(currBucketNum, currBucketPH)))
            return (rc);
    }
    return (0);
}

RC IX_IndexScan::GetAppropriateEntryInLeaf(PFPageHandle& leafPH) 
{
    RC rc = 0;
    hasLeafPinned = true;
    (char*&)leafHeader = leafPH.rawPtr();

    if (leafHeader->num_keys == 0)
        return (IX_EOF);

    leafEntries = (struct Node_Entry*)((char*)leafHeader + (indexHandle->header).entryOffset_N);
    leafKeys = (char*)leafHeader + (indexHandle->header).keysOffset_N;
    int index = 0;
    bool isDup = false;
    if ((rc = indexHandle->FindNodeInsertIndex((struct IX_NodeHeader*)leafHeader, value, index, isDup)))
        return (rc);

    leafSlot = index;
    if ((leafSlot != NO_MORE_SLOTS))
        nextNextKey = leafKeys + attrLength * leafSlot;
    else
        return (IX_INVALIDSCAN);

    if (leafEntries[leafSlot].isValid == OCCUPIED_DUP) { // if it's a duplciate value, go into the bucket
        currBucketNum = leafEntries[leafSlot].page;      // to retrieve the first entry
        if ((rc = GetFirstBucketEntry(currBucketNum, currBucketPH)))
            return (rc);
    }
    return (0);
}

/*
 * This function retrieve the first entry in a bucket given the BucketPageNum.
 */
RC IX_IndexScan::GetFirstBucketEntry(Page nextBucket, PFPageHandle& bucketPH) {
    RC rc = 0;
    if ((rc = (indexHandle->pfh).get()->getPage(nextBucket, currBucketPH))) // pin the bucket
        return (rc);
    hasBucketPinned = true;
    (char*&)bucketHeader = bucketPH.rawPtr();// retrieve bucket contents

    bucketEntries = (struct Bucket_Entry*)((char*)bucketHeader + (indexHandle->header).entryOffset_B);
    bucketSlot = bucketHeader->firstSlotIndex; // set the current scan to the first slot in bucket

    return (0);

}
