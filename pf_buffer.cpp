#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "pf_buffer.h"
#include "util.h"


using namespace RedBase;
using namespace std;

PFBuffer* PFBuffer::instance_ = nullptr;

//���Բ���LRU�㷨������pin����unpin������LRU�㷨����ɲ���redbase˹̹����ѧ��githubԴ��
// getPage - ��ȡһ��ָ�򻺴�ռ��ָ�룬���ҳ���Ѿ��ڻ���֮�еĻ���(re)pin ҳ�棬Ȼ�󷵻�һ��
//	ָ������ָ�롣���ҳ��û���ڻ����У�������ļ��ж�������pin it��Ȼ�󷵻�һ��ָ������ָ�롣���
//  ���������Ļ����滻��һ��ҳ��
//	
RC PFBuffer::getPage(FILE* fd, Page num, Ptr& addr)
{
	int idx = table_.search(fd, num);
	if (idx >= 0) 
	{
		// ҳ���Ѿ����ڴ����ˣ�����ֻ��Ҫ�������ü���
		nodes_[idx].count++;
		nodes_[idx].red = true;
	}
	else 
	{
		// ����һ���յ�ҳ��,ҳ�����buff�е�λ����idx��¼
		idx = searchAvaliableNode();
		if (idx < 0) return PF_NOBUF;
		//printf("readpage\n");
		readPage(fd, num, nodes_[idx].buffer);
		nodes_[idx].fd = fd;
		nodes_[idx].num = num;
		nodes_[idx].count = 1;
		nodes_[idx].dirty = false;
		nodes_[idx].red = true;
		table_.insert(fd, num, idx);
	}
	// dstָ��ֱ��ָ��ҳ���ײ�
	addr = nodes_[idx].buffer;
	return 0;
}

//
// pin - ��ҳ��̶��ڻ�������,�������̺�getPage����
//
RC PFBuffer::pin(FILE* fd, Page num)
{
	int idx = table_.search(fd, num);
	if (idx >= 0) 
	{
		// ҳ���Ѿ����ڴ����ˣ�����ֻ��Ҫ�������ü���
		//if (nodes_[idx].count == 0)
			
		nodes_[idx].count++;

	}
	else 
	{
		// ����һ���յ�ҳ��,ҳ�����buff�е�λ����idx��¼
		idx = searchAvaliableNode();
		if (idx < 0) return PF_NOBUF;
		readPage(fd, num, nodes_[idx].buffer);
		nodes_[idx].fd = fd;
		nodes_[idx].num = num;
		nodes_[idx].count = 1;
		nodes_[idx].dirty = false;
		table_.insert(fd, num, idx);
	}
	return 0;
}

//
// allocatePage - ����һ��ҳ��
// 
RC PFBuffer::allocPage(FILE* fd, Page num, Ptr& addr)//������������һ���������alloc��ʱ��
													//Ϊʲô���Ӵ���Ҳ����file�л�ö���ֱ�Ӵ�buffer
													//������Ϳ�������ϸ����һ����Ϊ����ÿһҳ��Ҫ
													//д���ļ����ԾͲ��ô��ļ��������˲�֪��߲���
													//�Ƿ����������������ط��������
{
	int idx = table_.search(fd, num);
	if (idx > 0) return PF_PAGEINBUF;

	// ����յ�ҳ
	idx = searchAvaliableNode();
	if (idx < 0) return PF_NOBUF;

	table_.insert(fd, num, idx);
	nodes_[idx].fd = fd;
	nodes_[idx].num = num;
	nodes_[idx].dirty = false;
	nodes_[idx].count = 1;
	nodes_[idx].red = true;
	addr = nodes_[idx].buffer;

	return 0;
}

//
// markDirty - ��һ��ҳ����Ϊ��,������bufffer�ж�����ʱ��,���ᱻ�ص��ļ���
// 
RC PFBuffer::markDirty(FILE* fd, Page num)
{
	int idx = table_.search(fd, num);
	// ҳ���������ڻ�����
	if (idx < 0)  return PF_PAGENOTINBUF;

	if (nodes_[idx].count == 0) return PF_PAGEUNPINNED;
	// ���Ϊ��ҳ
	nodes_[idx].dirty = true;
	return 0;
}

//
// unpin - ����һ��������
//	
RC PFBuffer::unpin(FILE* fd, Page num)
{
	int idx = table_.search(fd, num);
	if (idx < 0) return PF_PAGENOTINBUF;
	if (nodes_[idx].count == 0)
		return 0;
	nodes_[idx].count -= 1;//������Դ��Ļ����������޸Ľ����Ϊ�˴ӻ�������ɾ��ʱд��
	//if(nodes_[idx].count<0)
	//printf("num:%d count:%d\n", num, nodes_[idx].count);
	return 0;
}

//
// flush - �����ҳ��ȫ����ˢ��������
// 
RC PFBuffer::flush(FILE* fd)
{
	for (int i = 0; i < capacity; i++) {
		if (nodes_[i].fd == fd) {
			if (nodes_[i].dirty) {
				
				writeBack(fd, nodes_[i].num, nodes_[i].buffer);
				nodes_[i].dirty = false;
			}
		}
	}
	return 0;
}

//
// forcePages
// 
RC PFBuffer::forcePages(FILE* fd, Page num)
{
	for (int i = 0; i < capacity; i++) {
		if ((nodes_[i].fd == fd) && (nodes_[i].num == num)) {
			if (nodes_[i].dirty) 
			{
				writeBack(fd, nodes_[i].num, nodes_[i].buffer);
				nodes_[i].dirty = false;
			}
		}
	}
	return 0;
}

//
// clearFilePages - һ�����ļ�ǰ����,�����л����ҳ��ȫ���Ƴ�
//
void PFBuffer::clearFilePages(FILE* fd)
{
	for (int i = 0; i < capacity; i++) {
		if (nodes_[i].fd == fd) {
			table_.remove(nodes_[i].fd, nodes_[i].num);
			if (nodes_[i].dirty) {
				//printf("number:%d\n", i);
				writeBack(fd, nodes_[i].num, nodes_[i].buffer);
				nodes_[i].dirty = false;
			}
			nodes_[i].fd = NULL;
			nodes_[i].num = -1;
			nodes_[i].count = 0;
		}
	}
}

//
// searchAvaliableNode - �ڲ�ʹ�õĺ���
// 
int PFBuffer::searchAvaliableNode()
{
	int num = -1;
	for (int i = 0; i < capacity; i++) 
	{
		
		if (nodes_[i].red == false  && nodes_[i].count == 0) 
		{
			num = i;
			nodes_[i].red = false;
			break;
		}
		
		if (nodes_[i].count == 0)
		{
			nodes_[i].red = false;
		}
		/*else
			printf("%d,%d,%d\n", i,nodes_[i].num, nodes_[i].count);*/
	}
	if (num == -1)
	{
		for (int i = 0; i < capacity; i++)
		{
			if (nodes_[i].red == false && nodes_[i].count == 0)
			{
				num = i;
				break;
			}
		}
	}
	if (num == -1) return num;
	table_.remove(nodes_[num].fd, nodes_[num].num);
	
	if (nodes_[num].dirty)
	{
		writeBack(nodes_[num].fd, nodes_[num].num, nodes_[num].buffer);
		//printf("���ڵ�%d��%dҳ����\n",num,nodes_[num].num);
		nodes_[num].dirty = false;
	}
	
	return num;
}

//
// readPage - �Ӵ����ж�ȡһ��ҳ��
//	
RC PFBuffer::readPage(FILE* fd, Page num, Ptr dst)
{
	long offset = num * (long)pagesize + PF_FILE_HDR_SIZE;
	Lseek(fd, offset, 0);
	// ��ȡ����
	int n = Read(fd, dst, pagesize);
	if (n != pagesize) return PF_INCOMPLETEREAD;
	return 0; // һ������
}


//
// writeBack - ��src��������д�ص�����
// 
RC PFBuffer::writeBack(FILE* fd, Page num, Ptr src)
{
	// �����ҳ�����,���ļ�ͷ��֮��ʼ
	long offset = num * pagesize + PF_FILE_HDR_SIZE;
	Lseek(fd, offset, 0);
	
	int n = Write(fd, src, pagesize);
	if (n != pagesize) return PF_INCOMPLETEWRITE;
	return 0;
}


