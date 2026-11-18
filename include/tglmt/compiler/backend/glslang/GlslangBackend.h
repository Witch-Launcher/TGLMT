#pragma once

#include <memory>

#include "tglmt/compiler/ShaderIR.h"

namespace tglmt::compiler::backend::glslang {

std::unique_ptr<Compiler> CreateGlslangCompiler();

}
