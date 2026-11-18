#include "tglmt/compiler/passes/Canonicalization.h"

namespace tglmt::compiler::passes {

Canonicalization::Canonicalization()
    : Pass({"Canonicalization", "Chuan hoa IR sau semantic", true, true}) {}

bool Canonicalization::applicable(const ir::IRModule& ) const {
    return true;
}

PassResult Canonicalization::run(ir::IRModule& ) {
    return PassResult::OK;
}

} 
