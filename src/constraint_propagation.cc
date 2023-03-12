/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  ---
 *
 *
 */

#include "ctrd_prop.h"
#include "util.h"

using namespace z3;

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN


void collect_eq(RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
  bool use_ctrd_sig = false;
  bool use_const = false;
  int constValue;
  RTLIL::SigSpec outputWire;
  for(auto &conn: cell->connections_) {
    RTLIL::IdString port = conn.first;
    RTLIL::SigSpec connSig = conn.second;
    if(cell->input(port)) {
      if(connSig.is_wire() && connSig == ctrdSig) {
        use_ctrd_sig = true;
      }
      else if(connSig.is_fully_const()) {
        use_const = true;
        constValue = connSig.as_int();
      }
    }
    else {
      assert(cell->output(port));
      outputWire = connSig;
    }
  }
  if(use_ctrd_sig && use_const) {
    std::string path = get_path();
    g_check_vec.push_back(CheckSet{path, cell, outputWire, ctrdSig, constValue});
  }
}


void simplify_eq(solver &s, context &c, RTLIL::Module* module, 
                 RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig, int forbidValue) {
  print_cell(cell);
  bool use_ctrd_sig = false;
  bool use_forbid_value = false;
  RTLIL::SigSpec outputWire;
  RTLIL::IdString outputPort;
  for(auto &conn: cell->connections_) {
    RTLIL::IdString port = conn.first;
    RTLIL::SigSpec connSig = conn.second;
    print_sigspec(connSig);
    if(cell->input(port)) {
      if(connSig.is_wire() && connSig == ctrdSig) {
        use_ctrd_sig = true;
      }
      else if(connSig.is_fully_const()) {
        int eqValue = connSig.as_int();
        use_forbid_value = eqValue == forbidValue;
      }
    }
    else {
      assert(cell->output(port));
      outputWire = connSig;
      outputPort = port;
    }
  }
  // replace the eq with constant false
  if(use_ctrd_sig && use_forbid_value) {
    module->remove(cell);
    //cell->connections_[outputPort] = RTLIL::SigSpec();
    // connect the output wire to always false
    module->connect(outputWire, RTLIL::SigSpec(false));
  }
}


void add_submod(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
                RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
   RTLIL::SigSpec port = get_cell_port(ctrdSig, cell);
   if(port.empty()) return;
   auto subMod = get_subModule(design, cell);
   DriveMap_t mp;
   get_drive_map(subMod, mp);
   propagate_constraints(s, c, design, subMod, mp, port);
}


void add_and(solver &s, context &c, RTLIL::Design* design, RTLIL::Module* module, 
             DriveMap_t &mp, RTLIL::Cell* cell, RTLIL::SigSpec ctrdSig) {
  RTLIL::SigSpec port = get_cell_port(ctrdSig, cell);
  if(port.empty()) return;
  RTLIL::SigSpec outputConnSig;
  bool const_arg = false;
  int const_value;
  for(auto pair: cell->connections_) {
    auto portId = pair.first;
    auto connSig = pair.second;
    if(cell->input(portId) && connSig.is_fully_const()) {
      const_arg = true;
      const_value = connSig.as_int();
    }
    else if(cell->output(portId)) {
      outputConnSig = connSig;
    }
  }
  if(const_arg) {
    assert(equal_width(ctrdSig, outputConnSig));
    expr ctrdExpr = get_expr(c, ctrdSig);
    expr outExpr = get_expr(c, outputConnSig);
    s.add((ctrdExpr & const_value) == outExpr);
    propagate_constraints(s, c, design, module, mp, outputConnSig);
  }
}


void get_drive_map(RTLIL::Module* module, DriveMap_t &mp) {
  for(auto cellPair: module->cells_) {
    RTLIL::Cell* cell = cellPair.second;
    for(auto pair: cell->connections_) {
      auto portId = pair.first;
      auto connSig = pair.second;
      if(mp.find(connSig) == mp.end())
        mp.emplace(connSig, DestGroup{std::set<RTLIL::SigSpec>{}, std::set<RTLIL::Cell*>{cell}});
      else
        mp[connSig].cells.insert(cell);
    }
  }
}


/// Recursively propagate constraints through the design
void propagate_constraints(solver &s, context &c, Design* design, RTLIL::Module* module, 
                           DriveMap_t &mp, RTLIL::SigSpec ctrdSig)
                           //std::string ctrdSig, int offset, int length, uint32_t forbidValue)
{
  // traverse all connections
  //for(auto &conn : module->connections()) {
  //  auto srcSig = conn.first;
  //  //RTLIL::IdString srcName = srcSig.as_wire()->name;
  //  //if(ctdSig != srcSig) continue;
  //  auto dstSig = conn.second;
  //  if(dstSig.is_wire()) {
  //    g_work_list.push(std::make_pair(dstSig, forbidValue));
  //  }
  //}
  std::cout << "=== Begin a new module:"  << std::endl;
  print_module(module);
  // traverse all cells
  assert(mp.find(ctrdSig) != mp.end());
  const auto group = mp[ctrdSig];
  assert(group.wires.empty());
  auto connectedCells = group.cells;

  for(auto cell: connectedCells) {
    print_module(cell->module);
    if(cell->type == ID($eq)) 
      collect_eq(cell, ctrdSig);
    else if(cell_is_module(design, cell))
      add_submod(s, c, design, module, cell, ctrdSig);
    else if(cell->type == ID($and))
      add_and(s, c, design, module, mp, cell, ctrdSig);
  }
}


void simplify(solver &s, context &c) {
  for(auto set: g_check_vec) {
    std::string path = set.path;
    auto cell = set.cell;
    RTLIL::SigSpec outSig = set.outSig;
    RTLIL::SigSpec ctrdSig = set.ctrdSig;
    auto module = cell->module;
    int forbidValue = set.forbidValue;
    expr outExpr = get_expr(c, outSig, path);
    expr ctrdExpr = get_expr(c, ctrdSig, path);
    s.push();
    s.add(outExpr == (ctrdExpr == forbidValue));
    if(s.check() == unsat) {
      module->remove(cell);
      module->connect(outSig, RTLIL::SigSpec(false));
    }
    s.pop();
  }
}


struct ConstraintPropagatePass : public Pass {
  ConstraintPropagatePass() : Pass("opt_ctrd", "constraint propagation pass") { }
  void execute(std::vector<std::string>, Design* design) override { 
    log_header(design, "Executing the new OPT_CONSTRAINT pass\n");
    context c;
    solver s(c);
    // Iterate through all modules in the design
    RTLIL::Module* module = design->top_module();
    // Recursively propagate constants through the module
    std::string inputName = "\\io_opcode";
    int shift = 0;
    int length = 8;
    uint32_t forbidValue = 1;
    RTLIL::SigSpec inputSig = get_sigspec(module, inputName, shift, length);
    add_neq_ctrd(s, c, inputSig, forbidValue);
    DriveMap_t mp;
    get_drive_map(module, mp);
    propagate_constraints(s, c, design, module, mp, inputSig);
    simplify(s, c);
  }
} ConstraintPropagatePass;


PRIVATE_NAMESPACE_END
