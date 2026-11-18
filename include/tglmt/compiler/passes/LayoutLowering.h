#pragma once
#include "tglmt/compiler/passes/Pass.h"

namespace tglmt::compiler::passes {

class LayoutLowering : public Pass {
public:
    LayoutLowering();
    bool applicable(const ir::IRModule& module) const override;
    PassResult run(ir::IRModule& module) override;
};

} 
