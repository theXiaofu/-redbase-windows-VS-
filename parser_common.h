#pragma once
#include <iostream>
#include <list>
#include <regex>
#include <sstream>
#include <memory>
using namespace std;

//
// GeneralError��һ���ǳ����׵Ĵ�����,ֻ���ڼ�¼�������Ϣ,��������������׵�YACC��˵,�㹻��.
//
struct GeneralError {
public:
	string msg;
	GeneralError(const string& msg = "") :
		msg(msg)
	{}
public:
	string what() const {
		return msg;
	}
};

//
// GrammarError���ڼ�¼�ķ��Ĵ���. 
//
struct GrammarError : public GeneralError {
public:
	GrammarError(size_t line, const string& message = "")
		: GeneralError()
	{
		ostringstream grammarErr;
		grammarErr << "In line " << line << " : " << message << endl;
		msg = grammarErr.str();
	}
};