#include "redbase.h"
#include "sm_manager.h"
#include "pf_manager.h"
#include "ql_manager.h"
#include "ix.h"
using namespace std;

PFManager pfManager;
RMManager rmManager;

IX_Manager ixManager(pfManager);
SMManager smManager;
QLManager qlManager;

