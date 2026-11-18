#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "tglmt/compiler/ir/IR.h"
#include "tglmt/compiler/ShaderIR.h"

namespace tglmt::compiler::backend::metal {

struct MSLCompileOptions {
    bool precompile = true;          
    std::string cacheDir;            
    std::string metalArgs;           
    bool emitSourceOnly = false;     
};

struct MSLCompileResult {
    bool success = false;
    std::string mslSource;           
    std::vector<uint8_t> metallib;   
    std::vector<Diagnostic> diagnostics;
    
    
};

class MSLBackend {
public:
    MSLBackend();
    ~MSLBackend();

    
    std::string emitMSL(const ir::IRModule& module, MSLCompileResult& out) const;

    
    
    bool compileToMetallib(const std::string& mslSource,
                           const MSLCompileOptions& opts,
                           MSLCompileResult& out) const;

    
    MSLCompileResult build(const ir::IRModule& module,
                           const MSLCompileOptions& opts) const;

private:
    std::string typeName(const ir::IRType& t) const;
    std::string storageQualifier(ir::IRStorage s) const;
    std::string interpolationQualifier(ir::IRInterpolation i) const;
    bool hasMetal() const;           
    std::string escapeMSL(const std::string& s) const;
};

} 
