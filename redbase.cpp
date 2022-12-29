#include <iostream>
#include <string.h>
//#include <unistd.h>
#include "redbase.h"
#include "sm_manager.h"
#include "pf_manager.h"
#include "ql_manager.h"
#include "ix.h"
#include "parser.h"
using namespace std;

extern PFManager pfManager;
extern RMManager rmManager;
extern IX_Manager ixManager;
extern SMManager smManager;
extern QLManager qlManager;

int main(int argc, char** argv)
{
	string s;
	cout << "please type your dbname" << endl;
	cin >> s;
	if (s.length() <= 0) {
		cerr << "usage: " << s << " <dbname>\n";
		exit(1);
	}
	char dbname[20];
		strcpy(dbname,s.c_str());
	smManager.openDb(dbname);
	RBparse();
	smManager.closeDb();
	cout << "Bye.\n";
	return 0;
}
