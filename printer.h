#ifndef PRINTER_H
#define PRINTER_H

#include <iostream>
#include <string.h>
#include <vector>
#include "redbase.h"
#include "catalog.h"
#include "data_attr.h"

using namespace std;

#define MAXPRINTLEN ((2 * MAXNAME) + 5)


	//
// ItemDiscrip ���ڱ������
// 
struct ItemDiscrip;

class Printer {
public:
	Printer(const DataAttr* attrs, int nattrs);
	Printer(const ItemDiscrip* discs, int ndiscs);
	~Printer() {};
public:
	void printHeader();
	void print(uint8_t* data);
	void printFooter();
private:
	void printDiscs(uint8_t* data);
	void printAttrs(uint8_t* data);
private:
	const DataAttr* attrs_;
	int count_;
	const ItemDiscrip* discs_;
	int ndiscs_;
	vector<int> colWidth_;	/* ���ڼ�¼ÿһ��Ӧ������Ŀո����Ŀ */
	vector<char*> infos_;
	int printed_;			/* �Ѿ���������� */
};

#endif /* PRINTER_H */