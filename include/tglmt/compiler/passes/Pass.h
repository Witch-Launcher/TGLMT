#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "tglmt/compiler/ir/IR.h"

namespace tglmt::compiler::passes {

enum class PassResult {
    OK, Error, Skipped
};

struct PassInfo {
    std::string name;
    std::string description;
    bool modifiesIR = true;
    bool requiresVerifier = true;
};

class Pass {
public:
    explicit Pass(PassInfo info) : info_(std::move(info)) {}
    virtual ~Pass() = default;
    const PassInfo& info() const { return info_; }
    virtual PassResult run(ir::IRModule& module) = 0;
    virtual bool applicable(const ir::IRModule& module) const { return true; }
protected:
    PassInfo info_;
};

} 
