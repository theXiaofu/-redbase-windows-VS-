#include "pf.h"
#include "pf_filehandle.h"
#include "util.h"
#include "pf_manager.h"
#include "rm_manager.h"
#include <string>
#include "ix.h"
//#include "sm_manager.h"
PFManager pfManager;
std::string req_data(int i)
{
	std::string s;
	s.erase(s.begin(), s.end());
	s.append(std::to_string(i));
	s.append(" ");
	s.append("bank");
	s.append(std::to_string(i));
	s.append(" ");
	s.append(std::to_string(i));
	return s;
}
std::string req_data() {
	static int i = 10000;
	srand(time(0));
	char str1[20] = "a1a2a3";
	char str2[20] = "a4a5a6";
	char str3[20] = "a7a8a9";
	char* pstr[3] = { str1,str2,str3 };
	std::string s;
	s.erase(s.begin(), s.end());
	s.append(pstr[rand() % 3]);
	s.append(" ");
	s.append(std::to_string(i));
	i++;
	return s;
}
int main()
{
	RMManager rmmanager;
	RMFilePtr rmfileptr;
	rmmanager.createFile("db000", 1 << 20);
	rmmanager.openFile("db000",rmfileptr);
	char buf[100];
	char buf0[100];
	int page;
	int num;
	int len;
	IX_IndexHandle ixh;
	PFManager pfm;
	IX_Manager ixm(pfm);
	
	ixm.CreateIndex("db000", 1, INT0, 4);
	ixm.OpenIndex("db000", 1, ixh);
	char rec[100];
	char* addr = NULL;
	RID rid[10000];
	std::string s;
	for (int i = 0; i < 1000; ++i)
	{
		s = req_data();
		memset(buf, 0, sizeof(buf));
		strcpy(buf, s.data());
		rmfileptr.get()->insertRcd(buf, s.length() + 1, page, num);//插入索引
		rid[i].num = page;
		rid[i].st = num;
		int j = i + 10000;
		ixh.InsertEntry(&j, rid[i]);
		
	}
	IX_IndexScan ixscan;
	int j = 10086;
	ixscan.OpenScan(ixh, LT_OP, &j, NO_HINT);//获取其中大于9950的值
	//printf("新插入的页号：%d,页中的记录号：%d\n", page, num);
	RID rid0;
	for (int i = 0; i < 80; ++i)
	{
		//输出其中前30个大于9950的
		ixscan.GetNextEntry(rid0);
		rmfileptr.get()->getRcd(rid0.page(), rid0.slot(), addr, len);
		memcpy(rec, addr, len);
		printf("%s\n",rec);
	}
	rmmanager.closeFile(rmfileptr);
	//printf("123");
}