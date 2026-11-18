#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

#include "tglmt/compiler/AST.h"
#include "tglmt/compiler/Semantic.h"
#include "tglmt/compiler/ir/IR.h"

namespace tglmt::compiler::ir {

IRModule astToModule(const AstTranslationUnit& tu,
                     const SemanticResult& sem,
                     ShaderStage stage);

} 
