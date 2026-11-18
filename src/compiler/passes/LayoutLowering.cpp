#include "tglmt/compiler/passes/LayoutLowering.h"

namespace tglmt::compiler::passes {

LayoutLowering::LayoutLowering()
    : Pass({"LayoutLowering", "Dua resource binding sang Metal descriptor slot", true, true}) {}

bool LayoutLowering::applicable(const ir::IRModule& ) const {
    return true;
}

PassResult LayoutLowering::run(ir::IRModule& module) {
    int uniformSlot = 0;
    int samplerSlot = 0;
    int imageSlot = 0;
    int atomicSlot = 0;
    int ssboSlot = 0;

    for (auto& res : module.uniforms) {
        if (res.binding < 0) res.binding = uniformSlot++;
        if (res.set < 0) res.set = 0;
    }
    for (auto& res : module.samplers) {
        if (res.binding < 0) res.binding = samplerSlot++;
        if (res.set < 0) res.set = 0;
    }
    for (auto& res : module.images) {
        if (res.binding < 0) res.binding = imageSlot++;
        if (res.set < 0) res.set = 0;
    }
    for (auto& res : module.atomicCounters) {
        if (res.binding < 0) res.binding = atomicSlot++;
        if (res.set < 0) res.set = 0;
    }
    for (auto& res : module.storageBlocks) {
        if (res.binding < 0) res.binding = ssboSlot++;
        if (res.set < 0) res.set = 0;
    }
    for (auto& res : module.inputs) {
        if (res.location < 0) res.location = 0;
    }
    for (auto& res : module.outputs) {
        if (res.location < 0) res.location = 0;
    }

    return PassResult::OK;
}

} 
