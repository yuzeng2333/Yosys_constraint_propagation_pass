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
#include <queue>
#include <assert.h>


USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

std::queue<std::pair<RTLIL::SigSpec, uint32_t>> g_work_list;

/// utils
void print_cell(RTLIL::Cell* cell) {
  std::cout << "cell: " << cell->name.str() << std::endl;
}

void print_sigspec(RTLIL::SigSpec connSig) {
  if(connSig.is_wire()) {
    auto wire = connSig.as_wire();
    std::string wireName = wire->name.str();
    if(wireName == "\\io_opcode") std::cout << "find io_opcode" << std::endl;
    std::cout << "wire: " << wireName << std::endl;
  }
  else if(connSig.is_fully_const()) {
    std::cout << "const: " << connSig.as_int() << std::endl;    
  }
}

void print_IdString(RTLIL::IdString id) {
  std::cout << "IdString: " << id.str() << std::endl;
}

/// Recursively propagate constraints through the design
void propagate_constraints(Design* design, RTLIL::Module* module, 
                           std::string ctrdSig, uint32_t forbidValue=1)
{
    // traverse all connections
    for(auto &conn : module->connections()) {
      auto srcSig = conn.first;
      //RTLIL::IdString srcName = srcSig.as_wire()->name;
      //if(ctdSig != srcSig) continue;
      auto dstSig = conn.second;
      if(dstSig.is_wire()) {
        g_work_list.push(std::make_pair(dstSig, forbidValue));
      }
    }

    // traverse all cells
    for(auto cellPair : module->cells_) {
      RTLIL::IdString IdStr = cellPair.first;
      RTLIL::Cell* cell = cellPair.second;
      if(cell->type != ID($eq)) continue;
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
          if(connSig.is_wire()) {
            auto wire = connSig.as_wire();
            std::string wireName = wire->name.str();
            if(wireName == ctrdSig) use_ctrd_sig = true;
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
}

struct ConstraintPropagatePass : public Pass {
    ConstraintPropagatePass() : Pass("opt_ctrd", "constraint propagation pass") { }
    void execute(std::vector<std::string>, Design* design) override { 
        log_header(design, "Executing the new OPT_CONSTRAINT pass");
        // Iterate through all modules in the design
        for (auto module : design->modules())
        {
            // Recursively propagate constants through the module
            propagate_constraints(design, module, "\\io_opcode", 1);
        }
    }
} ConstraintPropagatePass;


PRIVATE_NAMESPACE_END
