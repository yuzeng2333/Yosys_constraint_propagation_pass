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
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <queue>


USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

std::queue<std::pair<RTLIL::SigSpec, uint32_t>> g_work_list;

// Recursively propagate constraints through the design
void propagate_constraints(Design* design, RTLIL::Module* module, 
                           RTLIL::SigSpec ctdSig, uint32_t forbidValue=0)
{
    for(auto &conn : module->connections_) {
      auto srcSig = conn.first;
      //RTLIL::IdString srcName = srcSig.as_wire()->name;
      if(ctdSig != srcSig) continue;
      auto dstSig = conn.second;
      if(dstSig.is_wire()) {
        g_work_list.push(std::make_pair(dstSig, forbidValue));
      }
      
    }
}

struct ConstraintPropagatePass : public Pass {
    ConstraintPropagatePass() : Pass("opt_ctrt", "constraint propagation pass") { }
    void execute(std::vector<std::string>, Design* design) override { 
        log_header(design, "Executing OPT_CONSTRAINT pass");
        // Iterate through all modules in the design
        for (auto module : design->modules())
        {
            // Recursively propagate constants through the module
            propagate_constraints(design, module, RTLIL::SigSpec());
        }
    }
} ConstraintPropagatePass;


PRIVATE_NAMESPACE_END
