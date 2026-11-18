#include "tglmt/compiler/ir/IR.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace tglmt::compiler::ir {

bool IRType::isNumeric() const {
    return scalar == IRScalar::Float || scalar == IRScalar::Double ||
           scalar == IRScalar::Int || scalar == IRScalar::UInt;
}
bool IRType::isInteger() const {
    return scalar == IRScalar::Int || scalar == IRScalar::UInt;
}
bool IRType::isFloat() const { return scalar == IRScalar::Float || scalar == IRScalar::Double; }
bool IRType::isBool() const { return scalar == IRScalar::Bool; }
bool IRType::isOpaque() const {
    return isSampler || isImage || kind == IRTypeKind::AtomicCounter ||
           kind == IRTypeKind::Opaque;
}

std::string IRType::toString() const {
    std::string s;
    switch (scalar) {
        case IRScalar::Float: s = "float"; break;
        case IRScalar::Double: s = "double"; break;
        case IRScalar::Int: s = "int"; break;
        case IRScalar::UInt: s = "uint"; break;
        case IRScalar::Bool: s = "bool"; break;
        case IRScalar::Void: s = "void"; break;
    }
    if (isSampler) s = "sampler";
    else if (isImage) s = "image";
    else if (kind == IRTypeKind::Vector) s = "vec" + std::to_string(vectorSize);
    else if (kind == IRTypeKind::Matrix) s = "mat" + std::to_string(matrixCols) + "x" + std::to_string(matrixRows);
    else if (kind == IRTypeKind::Array) s = "array<" + (arrayElement ? arrayElement->toString() : "?") + ">";
    else if (kind == IRTypeKind::Struct) s = "struct " + structName;
    return s;
}

bool IRType::operator==(const IRType& o) const {
    if (kind != o.kind) return false;
    if (scalar != o.scalar) return false;
    if (kind == IRTypeKind::Vector && vectorSize != o.vectorSize) return false;
    if (kind == IRTypeKind::Matrix && (matrixCols != o.matrixCols || matrixRows != o.matrixRows)) return false;
    if (kind == IRTypeKind::Array && arraySize != o.arraySize) return false;
    return true;
}

static bool isValidType(const IRType& t) {
    if (t.kind == IRTypeKind::Scalar || t.kind == IRTypeKind::Vector ||
        t.kind == IRTypeKind::Matrix) {
        return t.scalar != IRScalar::Void || t.kind == IRTypeKind::Scalar;
    }
    if (t.kind == IRTypeKind::Array) return t.arrayElement != nullptr;
    return true;
}

IRVerifyResult verifyBlock(const IRBasicBlock& block, const IRFunction& func) {
    IRVerifyResult r;
    r.valid = true;
    (void)func;
    for (const auto& inst : block.instructions) {
        if (!isValidType(inst.resultType)) {
            r.valid = false;
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.code = "IR-001";
            d.message = "instruction co kieu khong hop le";
            d.span = SourceSpan{};
            d.span->begin.line = inst.sourceLine > 0 ? inst.sourceLine : 1;
            d.span->begin.column = inst.sourceColumn > 0 ? inst.sourceColumn : 1;
            r.diagnostics.push_back(d);
        }
    }
    return r;
}

IRVerifyResult verifyFunction(const IRFunction& func) {
    IRVerifyResult r;
    r.valid = true;
    if (!isValidType(func.returnType)) {
        r.valid = false;
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.code = "IR-002";
        d.message = "ham " + func.name + " co kieu tra ve khong hop le";
        r.diagnostics.push_back(d);
    }
    for (const auto& block : func.blocks) {
        auto br = verifyBlock(block, func);
        if (!br.valid) { r.valid = false; r.diagnostics.insert(r.diagnostics.end(), br.diagnostics.begin(), br.diagnostics.end()); }
    }
    return r;
}

IRVerifyResult verifyModule(const IRModule& module) {
    IRVerifyResult r;
    r.valid = true;
    for (const auto& func : module.functions) {
        auto fr = verifyFunction(func);
        if (!fr.valid) { r.valid = false; r.diagnostics.insert(r.diagnostics.end(), fr.diagnostics.begin(), fr.diagnostics.end()); }
    }
    return r;
}

} 
