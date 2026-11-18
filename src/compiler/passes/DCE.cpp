#include "tglmt/compiler/passes/DCE.h"

#include <unordered_set>
#include <unordered_map>

namespace tglmt::compiler::passes {

DCE::DCE()
    : Pass({"DCE", "Xoa code chatchet khong co side effect", true, true}) {}

bool DCE::applicable(const ir::IRModule& ) const {
    return true;
}

static bool hasSideEffect(ir::IROp op) {
    switch (op) {
        case ir::IROp::Store: case ir::IROp::Call: case ir::IROp::Return:
        case ir::IROp::Discard: case ir::IROp::Barrier: case ir::IROp::MemoryBarrier:
        case ir::IROp::ImageLoad: case ir::IROp::ImageStore:
        case ir::IROp::AtomicAdd: case ir::IROp::AtomicSub: case ir::IROp::AtomicInc:
        case ir::IROp::AtomicDec: case ir::IROp::AtomicMin: case ir::IROp::AtomicMax:
        case ir::IROp::AtomicAnd: case ir::IROp::AtomicOr: case ir::IROp::AtomicXor:
        case ir::IROp::AtomicExchange: case ir::IROp::AtomicCompareExchange:
            return true;
        default:
            return false;
    }
}

PassResult DCE::run(ir::IRModule& module) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& func : module.functions) {
            for (auto& block : func.blocks) {
                std::unordered_set<int> usedIds;
                for (const auto& inst : block.instructions) {
                    for (int op : inst.operandIds) {
                        if (op >= 0) usedIds.insert(op);
                    }
                }
                for (size_t i = 0; i < block.instructions.size(); ) {
                    auto& inst = block.instructions[i];
                    if (inst.resultId < 0 || hasSideEffect(inst.op) || usedIds.count(inst.resultId)) {
                        ++i;
                        continue;
                    }
                    block.instructions.erase(block.instructions.begin() + static_cast<long>(i));
                    changed = true;
                }
            }
        }
    }
    return PassResult::OK;
}

} 
