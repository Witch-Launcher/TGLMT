#pragma once
#include "tglmt/compiler/passes/Pass.h"

namespace tglmt::compiler::passes {

class DCE : public Pass {
public:
    DCE();
    bool applicable(const ir::IRModule& module) const override;
    PassResult run(ir::IRModule& module) override;
};

} 
