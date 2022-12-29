#ifndef PARSER_LEXER_H
#define PARSER_LEXER_H

#include <regex>
#include <set>
#include <list>
#include <memory>
#include "parser_common.h"
#include "parser_token.h"
using namespace std;
struct TokenDef;


class Lexer;
using LexerPtr = shared_ptr<Lexer>;
using TokenPtr = shared_ptr<Token>;

//
// Lexer��һ���ʷ�������.
//
class Lexer
{
public:
	Lexer();
public:
	~Lexer();
private:
	string stream_;
	size_t offset_;					// ���ڼ�¼�Ѿ��������˵�λ��
	size_t line_;					// ��¼�ı����ڵ���,��Ҳ�ǳ���Ҫ
	bool eof_;
public:
	void setStream(const string& stream) {
		offset_ = 0;
		line_ = 1;				// �ӵ�1�п�ʼ
		stream_ = stream;
		eof_ = stream.size() == 0;
	}
	TokenPtr next();
private:
	int countNL(const string&);
private:
	static LexerPtr instance_;
public:
	static LexerPtr instance();
};

#endif /* PARSER_LEXER_H */
