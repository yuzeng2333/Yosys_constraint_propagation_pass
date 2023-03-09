#ifndef CTRD_PROP
#define CTRD_PROP

#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/log.h"
#include "kernel/sigtools.h"
#include "kernel/ff.h"
#include "kernel/mem.h"
#include "kernel/rtlil.h"
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <assert.h>
#include <z3++.h>

struct WorkItem {
  std::string sigName;
  int offset;
  int length;
  uint32_t forbidValue;
};


struct CheckSet {
  std::string path;
  RTLIL::Cell* cell;
  RTLIL::SigSpec ctrdSig;
  int forbidValue;
};


struct DestGroup {
  std::set<RTLIL::SigSpec> wires;
  std::set<std::pair<RTLIL:Cell*, RTLIL::SigSpec>> cellPorts;
};

std::queue<WorkItem> g_work_list;
std::vector<RTLIL::Cell*> g_cell_stack;
std::set<CheckSet> g_check_set;
std::map<std::string, expr> g_expr_map;
typedef std::map<RTLIL::SigSpec, DestGroup> DriveMap_t;


#endif
