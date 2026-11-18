#include "tglmt/compiler/passes/ConstantFolding.h"

#include <cmath>
#include <unordered_map>

namespace tglmt::compiler::passes {

ConstantFolding::ConstantFolding()
    : Pass({"ConstantFolding", "Tinh toan bieu thuc hang so o thoi dien", true, true}) {}

bool ConstantFolding::applicable(const ir::IRModule& ) const {
    return true;
}

static bool isConstOp(ir::IROp op) {
    switch (op) {
        case ir::IROp::Add: case ir::IROp::Sub: case ir::IROp::Mul:
        case ir::IROp::Div: case ir::IROp::Mod: case ir::IROp::Neg:
        case ir::IROp::Not: case ir::IROp::Eq: case ir::IROp::Ne:
        case ir::IROp::Lt: case ir::IROp::Le: case ir::IROp::Gt:
        case ir::IROp::Ge: case ir::IROp::And: case ir::IROp::Or:
            return true;
        default:
            return false;
    }
}

static bool evalConst(ir::IROp op, const ir::IRConstant& a, const ir::IRConstant& b, ir::IRConstant& out) {
    if (a.isInt && b.isInt) {
        long long av = a.intValue, bv = b.intValue;
        switch (op) {
            case ir::IROp::Add: out.intValue = av + bv; out.isInt = true; return true;
            case ir::IROp::Sub: out.intValue = av - bv; out.isInt = true; return true;
            case ir::IROp::Mul: out.intValue = av * bv; out.isInt = true; return true;
            case ir::IROp::Div: if (bv == 0) return false; out.intValue = av / bv; out.isInt = true; return true;
            case ir::IROp::Mod: if (bv == 0) return false; out.intValue = av % bv; out.isInt = true; return true;
            case ir::IROp::Eq: out.intValue = (av == bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Ne: out.intValue = (av != bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Lt: out.intValue = (av < bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Le: out.intValue = (av <= bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Gt: out.intValue = (av > bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Ge: out.intValue = (av >= bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::And: out.intValue = (av && bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Or: out.intValue = (av || bv) ? 1 : 0; out.isInt = true; return true;
            default: return false;
        }
    } else if (!a.isInt && !b.isInt) {
        double av = a.floatValue, bv = b.floatValue;
        switch (op) {
            case ir::IROp::Add: out.floatValue = av + bv; out.isInt = false; return true;
            case ir::IROp::Sub: out.floatValue = av - bv; out.isInt = false; return true;
            case ir::IROp::Mul: out.floatValue = av * bv; out.isInt = false; return true;
            case ir::IROp::Div: if (bv == 0.0) return false; out.floatValue = av / bv; out.isInt = false; return true;
            case ir::IROp::Mod: if (bv == 0.0) return false; out.floatValue = std::fmod(av, bv); out.isInt = false; return true;
            case ir::IROp::Eq: out.intValue = (av == bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Ne: out.intValue = (av != bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Lt: out.intValue = (av < bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Le: out.intValue = (av <= bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Gt: out.intValue = (av > bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Ge: out.intValue = (av >= bv) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::And: out.intValue = (av != 0.0 && bv != 0.0) ? 1 : 0; out.isInt = true; return true;
            case ir::IROp::Or: out.intValue = (av != 0.0 || bv != 0.0) ? 1 : 0; out.isInt = true; return true;
            default: return false;
        }
    }
    return false;
}

static bool evalUnaryConst(ir::IROp op, const ir::IRConstant& a, ir::IRConstant& out) {
    if (a.isInt) {
        long long av = a.intValue;
        switch (op) {
            case ir::IROp::Neg: out.intValue = -av; out.isInt = true; return true;
            case ir::IROp::Not: out.intValue = (!av) ? 1 : 0; out.isInt = true; return true;
            default: return false;
        }
    } else {
        double av = a.floatValue;
        switch (op) {
            case ir::IROp::Neg: out.floatValue = -av; out.isInt = false; return true;
            case ir::IROp::Not: out.intValue = (av == 0.0) ? 1 : 0; out.isInt = true; return true;
            default: return false;
        }
    }
}

PassResult ConstantFolding::run(ir::IRModule& module) {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& func : module.functions) {
            for (auto& block : func.blocks) {
                std::unordered_map<int, ir::IRConstant> constValues;
                for (const auto& kv : module.constantValues) {
                    constValues[kv.first] = kv.second;
                }
                for (size_t i = 0; i < block.instructions.size(); ) {
                    auto& inst = block.instructions[i];
                    if (!isConstOp(inst.op) || inst.resultId < 0 || inst.operandIds.empty()) {
                        ++i;
                        continue;
                    }
                    if (inst.op == ir::IROp::Neg || inst.op == ir::IROp::Not) {
                        if (inst.operandIds.size() != 1) { ++i; continue; }
                        auto cit = constValues.find(inst.operandIds[0]);
                        if (cit == constValues.end()) { ++i; continue; }
                        ir::IRConstant out;
                        if (!evalUnaryConst(inst.op, cit->second, out)) { ++i; continue; }
                        constValues[inst.resultId] = out;
                        module.constantValues[inst.resultId] = out;
                        block.instructions.erase(block.instructions.begin() + static_cast<long>(i));
                        changed = true;
                        continue;
                    }
                    if (inst.operandIds.size() != 2) { ++i; continue; }
                    auto cit1 = constValues.find(inst.operandIds[0]);
                    auto cit2 = constValues.find(inst.operandIds[1]);
                    if (cit1 == constValues.end() || cit2 == constValues.end()) { ++i; continue; }
                    ir::IRConstant out;
                    if (!evalConst(inst.op, cit1->second, cit2->second, out)) { ++i; continue; }
                    constValues[inst.resultId] = out;
                    module.constantValues[inst.resultId] = out;
                    block.instructions.erase(block.instructions.begin() + static_cast<long>(i));
                    changed = true;
                    continue;
                }
            }
        }
    }
    return PassResult::OK;
}

} 
