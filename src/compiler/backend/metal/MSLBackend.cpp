#include "tglmt/compiler/backend/metal/MSLBackend.h"

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <unistd.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace tglmt::compiler::backend::metal {

namespace {

std::string scalarName(ir::IRScalar s) {
    switch (s) {
        case ir::IRScalar::Float: return "float";
        case ir::IRScalar::Double: return "float";
        case ir::IRScalar::Int: return "int";
        case ir::IRScalar::UInt: return "uint";
        case ir::IRScalar::Bool: return "bool";
        case ir::IRScalar::Void: return "void";
    }
    return "float";
}

std::string convertSwizzle(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case 'r': out += 'x'; break;
            case 'g': out += 'y'; break;
            case 'b': out += 'z'; break;
            case 'a': out += 'w'; break;
            case 's': out += 'x'; break;
            case 't': out += 'y'; break;
            case 'p': out += 'z'; break;
            case 'q': out += 'w'; break;
            default: out += c; break;
        }
    }
    return out;
}

bool looksLikeSwizzle(const std::string& s) {
    if (s.empty() || s.size() > 4) return false;
    for (char c : s) {
        if (!strchr("xyzwrgbastpq", c)) return false;
    }
    return true;
}

std::string floatLiteral(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.9g", v);
    std::string s(buf);
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find("inf") == std::string::npos && s.find("nan") == std::string::npos)
        s += ".0";
    if (s == "-0.0" || s == "-0") s = "0.0";
    return s + "f";
}

struct Emitter {
    const ir::IRModule& module;
    std::unordered_map<std::string, std::string> varMap;
    std::vector<std::unordered_map<std::string, std::string>> scopes;
    std::vector<std::string> mutableGlobals;
    std::unordered_map<std::string, const ir::IRFunction*> funcByName;
    std::set<std::string> helpers;
    bool isFragment = false;
    bool usesPointSize = false;
    bool usesFrontFacing = false;
    bool usesPointCoord = false;
    bool usesFragCoord = false;
    bool usesFragDepth = false;
    bool usesVertexId = false;
    bool usesInstanceId = false;

    explicit Emitter(const ir::IRModule& m) : module(m) {
        isFragment = m.stage == ShaderStage::Fragment;
        for (const auto& f : m.functions) {
            funcByName[f.name] = &f;
        }
    }

    std::string typeName(const ir::IRType& t) const {
        if (t.isSampler || t.isImage) {
            std::string elem = scalarName(t.scalar);
            if (t.isSampler && t.samplerDim == 9) return "device const " + elem + "4*";
            if (t.isShadow) return "depth2d<float>";
            switch (t.samplerDim) {
                case 1: return "texture1d<" + elem + ">";
                case 2: return "texture2d<" + elem + ">";
                case 3: return "texture3d<" + elem + ">";
                case 4: return "texturecube<" + elem + ">";
                case 6: return "texture1d_array<" + elem + ">";
                case 7: return "texture2d_array<" + elem + ">";
                case 8: return "texturecube_array<" + elem + ">";
                case 10: return "texture2d_ms<" + elem + ">";
                case 11: return "texture2d_ms_array<" + elem + ">";
                case 5: return "texture2d<" + elem + ">";
                default: return "texture2d<" + elem + ">";
            }
        }
        switch (t.kind) {
            case ir::IRTypeKind::Scalar: return scalarName(t.scalar);
            case ir::IRTypeKind::Vector: {
                std::string base = scalarName(t.scalar);
                return base + std::to_string(t.vectorSize);
            }
            case ir::IRTypeKind::Matrix: {
                return "float" + std::to_string(t.matrixCols) + "x" + std::to_string(t.matrixRows);
            }
            case ir::IRTypeKind::Array: {
                std::string elem = t.arrayElement ? typeName(*t.arrayElement) : "float";
                int n = t.arraySize > 0 ? t.arraySize : 1;
                return "array<" + elem + ", " + std::to_string(n) + ">";
            }
            case ir::IRTypeKind::Struct:
                return t.structName.empty() ? "TglmtStruct" : t.structName;
            default:
                return "float";
        }
    }

    std::string zeroValue(const ir::IRType& t) const {
        if (t.kind == ir::IRTypeKind::Struct) return typeName(t);
        std::string tn = typeName(t);
        if (tn == "void") return "";
        return tn + "(0)";
    }

    bool isUserFunction(const std::string& name) const {
        return funcByName.count(name) != 0;
    }

    const ir::IRFunction* findFunction(const std::string& name, size_t argc) const {
        auto range = funcByName.equal_range(name);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second->paramTypes.size() == argc) return it->second;
        }
        auto it = funcByName.find(name);
        return it != funcByName.end() ? it->second : nullptr;
    }

    std::string extraArgs() const {
        std::string out;
        for (const auto& u : module.uniforms) out += ", tglmt_u_" + u.name;
        for (const auto& s : module.storageBlocks) out += ", tglmt_u_" + s.name;
        for (const auto& s : module.samplers) out += ", tglmt_u_" + s.name;
        for (const auto& s : module.images) out += ", tglmt_u_" + s.name;
        for (const auto& g : mutableGlobals) out += ", " + g;
        return out;
    }

    void pushScope() { scopes.emplace_back(); }
    void popScope() { if (!scopes.empty()) scopes.pop_back(); }
    void bindParam(const std::string& name) { scopes.back()[name] = name; }
    std::string declareLocal(const std::string& name) {
        std::string renamed = "tglmt_l_" + name;
        if (!scopes.empty()) scopes.back()[name] = renamed;
        return renamed;
    }

    static ir::IRType numericAs(const ir::IRType& t, ir::IRScalar s) {
        ir::IRType r = t;
        r.scalar = s;
        r.kind = t.kind;
        return r;
    }

    std::string castTo(const std::string& text, const ir::IRType& from, const ir::IRType& to) const {
        if (!from.isNumeric() || !to.isNumeric()) return text;
        if (from.kind == to.kind && from.scalar == to.scalar &&
            from.vectorSize == to.vectorSize &&
            from.matrixCols == to.matrixCols && from.matrixRows == to.matrixRows)
            return text;
        if (to.isScalar() && from.isScalar())
            return typeName(to) + "(" + text + ")";
        if (to.isVector() && from.isScalar())
            return typeName(to) + "(" + text + ")";
        if (to.isVector() && from.isVector() && from.vectorSize == to.vectorSize)
            return typeName(to) + "(" + text + ")";
        if (to.isMatrix() && from.isMatrix() &&
            from.matrixCols == to.matrixCols && from.matrixRows == to.matrixRows)
            return typeName(to) + "(" + text + ")";
        return text;
    }

    std::string expr(const std::shared_ptr<ir::IRExpr>& e) {
        if (!e) return "";
        switch (e->kind) {
            case ir::IRExpr::Kind::Constant: {
                if (e->type.scalar == ir::IRScalar::Bool || e->type.kind == ir::IRTypeKind::Bool)
                    return e->constant.intValue ? "true" : "false";
                if (e->type.scalar == ir::IRScalar::Int) return std::to_string(e->constant.intValue);
                if (e->type.scalar == ir::IRScalar::UInt)
                    return std::to_string((unsigned long long)e->constant.intValue) + "u";
                return floatLiteral(e->constant.floatValue);
            }
            case ir::IRExpr::Kind::Variable:
                return variableRef(e->text);
            case ir::IRExpr::Kind::Unary:
                return "(" + e->text + expr(e->args[0]) + ")";
            case ir::IRExpr::Kind::PrefixInc:
                return "(++" + expr(e->args[0]) + ")";
            case ir::IRExpr::Kind::PrefixDec:
                return "(--" + expr(e->args[0]) + ")";
            case ir::IRExpr::Kind::PostfixInc:
                return "(" + expr(e->args[0]) + "++)";
            case ir::IRExpr::Kind::PostfixDec:
                return "(" + expr(e->args[0]) + "--)";
            case ir::IRExpr::Kind::Binary: {
                std::string l = expr(e->args[0]);
                std::string r = expr(e->args[1]);
                auto& lt = e->args[0]->type;
                auto& rt = e->args[1]->type;
                bool arith = e->text == "+" || e->text == "-" || e->text == "*" ||
                             e->text == "/" || e->text == "%";
                if (arith && e->type.isNumeric() && e->type.kind != ir::IRTypeKind::Matrix &&
                    !lt.isMatrix() && !rt.isMatrix()) {
                    if (lt.isNumeric() && !lt.isOpaque())
                        l = castTo(l, lt, numericAs(lt, e->type.scalar));
                    if (rt.isNumeric() && !rt.isOpaque())
                        r = castTo(r, rt, numericAs(rt, e->type.scalar));
                } else if (lt.isFloat() && rt.isInteger()) {
                    r = castTo(r, rt, numericAs(rt, ir::IRScalar::Float));
                } else if (rt.isFloat() && lt.isInteger()) {
                    l = castTo(l, lt, numericAs(lt, ir::IRScalar::Float));
                }
                return "(" + l + " " + e->text + " " + r + ")";
            }
            case ir::IRExpr::Kind::Assign: {
                std::string rhs = expr(e->args[1]);
                if (e->text == "=" && e->args[0]) {
                    std::string lhs = expr(e->args[0]);
                    std::string rh = expr(e->args[1]);
                    rh = castTo(rh, e->args[1]->type, e->args[0]->type);
                    return "(" + lhs + " = " + rh + ")";
                }
                return "(" + expr(e->args[0]) + " " + e->text + " " + rhs + ")";
            }
            case ir::IRExpr::Kind::Ternary: {
                std::string t = expr(e->args[1]);
                std::string f = expr(e->args[2]);
                if (e->type.isNumeric() && e->args[1] && e->args[2]) {
                    auto& tt = e->args[1]->type;
                    auto& ft = e->args[2]->type;
                    if (tt.isNumeric() && !tt.isOpaque() && !tt.isMatrix())
                        t = castTo(t, tt, numericAs(tt, e->type.scalar));
                    if (ft.isNumeric() && !ft.isOpaque() && !ft.isMatrix())
                        f = castTo(f, ft, numericAs(ft, e->type.scalar));
                }
                return "(" + expr(e->args[0]) + " ? " + t + " : " + f + ")";
            }
            case ir::IRExpr::Kind::Member: {
                std::string obj = expr(e->args[0]);
                std::string name = e->text;
                if (e->args[0] && e->args[0]->type.kind == ir::IRTypeKind::Vector &&
                    looksLikeSwizzle(name))
                    name = convertSwizzle(name);
                return obj + "." + name;
            }
            case ir::IRExpr::Kind::Swizzle:
                return expr(e->args[0]) + "." + convertSwizzle(e->text);
            case ir::IRExpr::Kind::Index:
                return expr(e->args[0]) + "[" + expr(e->args[1]) + "]";
            case ir::IRExpr::Kind::Call:
                return callExpr(e);
            case ir::IRExpr::Kind::Constructor:
                return constructorExpr(e);
        }
        return "";
    }

    std::string variableRef(const std::string& name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(name);
            if (f != it->end()) return f->second;
        }
        auto it = varMap.find(name);
        if (it != varMap.end()) return it->second;
        if (name == "gl_Position") return "gl_Position";
        if (name == "gl_PointSize") { usesPointSize = true; return "gl_PointSize"; }
        if (name == "gl_VertexID") { usesVertexId = true; return "gl_VertexID"; }
        if (name == "gl_InstanceID") { usesInstanceId = true; return "gl_InstanceID"; }
        if (name == "gl_FragCoord") { usesFragCoord = true; return "_in.tglmt_gl_FragCoord"; }
        if (name == "gl_FrontFacing") { usesFrontFacing = true; return "gl_FrontFacing"; }
        if (name == "gl_PointCoord") { usesPointCoord = true; return "gl_PointCoord"; }
        if (name == "gl_FragDepth") { usesFragDepth = true; return "gl_FragDepth"; }
        return name;
    }

    std::string sampleState() const { return "tglmt_linear"; }

    std::string callExpr(const std::shared_ptr<ir::IRExpr>& e) {
        const std::string& name = e->text;
        auto a = [&](size_t i) { return expr(e->args[i]); };

        if (name == "texture" && e->args.size() == 2)
            return a(0) + ".sample(" + sampleState() + ", " + a(1) + ")";
        if (name == "texture" && e->args.size() >= 3)
            return a(0) + ".sample(" + sampleState() + ", " + a(1) + ")";
        if (name == "textureLod" && e->args.size() == 3)
            return a(0) + ".sample(" + sampleState() + ", " + a(1) + ", level(" + a(2) + "))";
        if (name == "textureGrad" && e->args.size() == 4)
            return a(0) + ".sample(" + sampleState() + ", " + a(1) + ", gradient2d(" +
                   a(2) + ", " + a(3) + "))";
        if (name == "textureProj" && e->args.size() == 2) {
            std::string uv = a(1);
            auto geometric = e->args[1]->type.kind == ir::IRTypeKind::Vector
                                 ? e->args[1]->type.vectorSize
                                 : 4;
            std::string denom = geometric >= 4 ? "(" + uv + ").w" : "(" + uv + ").z";
            return a(0) + ".sample(" + sampleState() + ", (" + uv + ").xy / " + denom + ")";
        }
        if (name == "texelFetch") {
            if (e->args.size() == 2) {
                auto st = e->args[0] ? e->args[0]->type : ir::IRType{};
                if (st.isSampler && st.samplerDim == 9)
                    return a(0) + "[uint(" + a(1) + ")]";
                std::string coord = a(1);
                if (e->args[1] && e->args[1]->type.kind == ir::IRTypeKind::Vector &&
                    e->args[1]->type.vectorSize > 1)
                    coord = "uint" + std::to_string(e->args[1]->type.vectorSize) + "(" + coord + ")";
                else
                    coord = "uint2(" + coord + ")";
                return a(0) + ".read(" + coord + ")";
            }
            if (e->args.size() == 3) {
                std::string coord = a(1);
                if (e->args[1] && e->args[1]->type.kind == ir::IRTypeKind::Vector)
                    coord = "uint" + std::to_string(e->args[1]->type.vectorSize) + "(" + coord + ")";
                else
                    coord = "uint2(" + coord + ")";
                return a(0) + ".read(" + coord + ", uint(" + a(2) + "))";
            }
        }
        if (name == "textureSize" && e->args.size() == 1) {
            auto st = e->args[0] ? e->args[0]->type : ir::IRType{};
            std::string s = a(0);
            if (st.isSampler && st.samplerDim == 8) return "int2(" + s + ".get_width(), " + s + ".get_height())";
            if (st.isSampler && st.samplerDim == 7)
                return "int3(" + s + ".get_width(), " + s + ".get_height(), " + s + ".get_array_size())";
            if (st.isSampler && st.samplerDim == 6)
                return "int2(" + s + ".get_width(), " + s + ".get_array_size())";
            if (st.isSampler && st.samplerDim == 3)
                return "int3(" + s + ".get_width(), " + s + ".get_height(), " + s + ".get_depth())";
            if (st.isSampler && st.samplerDim == 4) return "int2(" + s + ".get_width(), " + s + ".get_height())";
            return "int2(" + s + ".get_width(), " + s + ".get_height())";
        }
        if (name == "mod" && e->args.size() == 2) {
            std::string ty = typeName(e->args[0] ? e->args[0]->type : e->args[1]->type);
            helpers.insert("tglmt_mod_" + ty);
            return "tglmt_mod_" + ty + "(" + a(0) + ", " + a(1) + ")";
        }
        if (name == "atan" && e->args.size() == 2) return "atan2(" + a(0) + ", " + a(1) + ")";
        if (name == "radians" || name == "degrees") {
            std::string fn = "tglmt_" + name + "_" + typeName(e->args[0] ? e->args[0]->type
                                                                       : ir::IRType{});
            helpers.insert(fn);
            return fn + "(" + a(0) + ")";
        }
        if (name == "inversesqrt") return "rsqrt(" + a(0) + ")";
        if (name == "dFdx") return "dfdx(" + a(0) + ")";
        if (name == "dFdy") return "dfdy(" + a(0) + ")";
        if (name == "fwidth") return "fwidth(" + a(0) + ")";
        if (name == "textureQueryLod") return "float2(0.0)";
        if (name == "textureQueryLevels") return "int(1)";
        if (name == "bitfieldExtract" && e->args.size() == 3)
            return "extract_bits(" + a(0) + ", uint(" + a(1) + "), uint(" + a(2) + "))";
        if (name == "packHalf2x16") return "as_type<uint>(half2(" + a(0) + "))";

        if (isUserFunction(name)) {
            const ir::IRFunction* f = findFunction(name, e->args.size());
            std::string out = name + "(";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) out += ", ";
                std::string arg = a(i);
                if (f && i < f->paramTypes.size() && e->args[i])
                    arg = castTo(arg, e->args[i]->type, f->paramTypes[i]);
                out += arg;
            }
            out += extraArgs() + ")";
            return out;
        }

        std::string out = name + "(";
        if (name == "roundEven") out = "round(";
        for (size_t i = 0; i < e->args.size(); ++i) {
            if (i) out += ", ";
            out += a(i);
        }
        out += ")";
        return out;
    }

    std::string matrixResize(const std::string& srcText, const ir::IRType& from,
                             const ir::IRType& to) {
        std::string helper = "tglmt_mat" + std::to_string(to.matrixCols) + "x" +
                             std::to_string(to.matrixRows) + "_from_mat" +
                             std::to_string(from.matrixCols) + "x" +
                             std::to_string(from.matrixRows);
        helpers.insert(helper);
        return helper + "(" + srcText + ")";
    }

    std::string constructorExpr(const std::shared_ptr<ir::IRExpr>& e) {
        const std::string& name = e->text;
        auto a = [&](size_t i) { return expr(e->args[i]); };

        if (e->type.kind == ir::IRTypeKind::Array) {
            std::string elem = e->type.arrayElement ? typeName(*e->type.arrayElement)
                                                    : typeName(ir::IRType{});
            int n = e->type.arraySize > 0 ? e->type.arraySize : (int)e->args.size();
            std::string out = "array<" + elem + ", " + std::to_string(n) + ">{";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) out += ", ";
                out += a(i);
            }
            out += "}";
            return out;
        }

        ir::IRType target = e->type;
        if (target.kind == ir::IRTypeKind::Struct || target.structName == name) {
            std::string out = typeName(target) + "{";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) out += ", ";
                out += a(i);
            }
            out += "}";
            return out;
        }

        if (target.kind == ir::IRTypeKind::Matrix) {
            std::string ty = typeName(target);
            if (e->args.empty()) return ty + "(1.0)";
            if (e->args.size() == 1) {
                auto& arg = e->args[0];
                if (arg && arg->type.kind == ir::IRTypeKind::Matrix)
                    return matrixResize(a(0), arg->type, target);
                return ty + "(" + a(0) + ")";
            }
            if (e->args.size() == target.matrixCols) {
                std::string out = ty + "(";
                for (size_t i = 0; i < e->args.size(); ++i) {
                    if (i) out += ", ";
                    out += a(i);
                }
                out += ")";
                return out;
            }
            int cols = target.matrixCols, rows = target.matrixRows;
            if ((int)e->args.size() == cols * rows) {
                std::string out = ty + "(";
                for (int c = 0; c < cols; ++c) {
                    if (c) out += ", ";
                    out += "float" + std::to_string(rows) + "(";
                    for (int r = 0; r < rows; ++r) {
                        if (r) out += ", ";
                        out += a((size_t)(c * rows + r));
                    }
                    out += ")";
                }
                out += ")";
                return out;
            }
            return ty + "(" + a(0) + ")";
        }

        std::string ty = typeName(target);
        if (ty == "void") ty = "float";
        std::string out = ty + "(";
        for (size_t i = 0; i < e->args.size(); ++i) {
            if (i) out += ", ";
            std::string arg = a(i);
            if (e->args[i] && target.scalar == ir::IRScalar::Float &&
                e->args[i]->type.scalar != ir::IRScalar::Float &&
                e->args[i]->type.isNumeric() && !e->args[i]->type.isBool())
                arg = castTo(arg, e->args[i]->type,
                             numericAs(e->args[i]->type, ir::IRScalar::Float));
            out += arg;
        }
        out += ")";
        return out;
    }
};

std::string emitMutableGlobals(std::ostringstream& os, Emitter& em) {
    for (const auto& g : em.module.globals) {
        if (g.storage == ir::IRStorage::Constant) continue;
        std::string ty = em.typeName(g.type);
        if (g.type.kind == ir::IRTypeKind::Array && g.type.arraySize <= 0 && g.init &&
            g.init->type.kind == ir::IRTypeKind::Array)
            ty = em.typeName(g.init->type);
        os << "    " << ty << " " << g.name;
        if (g.init) os << " = " << em.expr(g.init);
        else if (g.type.kind == ir::IRTypeKind::Array) os << " = {}";
        else os << " = " << ty << "(0)";
        os << ";\n";
    }
    return "";
}

void emitStmt(std::ostringstream& os, Emitter& em, const std::shared_ptr<ir::IRStmt>& s,
              int indent);

void indentTo(std::ostringstream& os, int n) {
    for (int i = 0; i < n; ++i) os << "    ";
}

std::string stmtInline(Emitter& em, const std::shared_ptr<ir::IRStmt>& s) {
    if (!s) return "";
    if (s->kind == ir::IRStmt::Kind::Declaration) {
        std::string ty = em.typeName(s->declType);
        if (s->init && s->declType.kind == ir::IRTypeKind::Array &&
            s->declType.arraySize <= 0 && s->init->type.kind == ir::IRTypeKind::Array) {
            ty = em.typeName(s->init->type);
        }
        std::string init = s->init ? " = " + em.expr(s->init) : "";
        return ty + " " + em.declareLocal(s->declName) + init;
    }
    if (s->kind == ir::IRStmt::Kind::Expression) return em.expr(s->expr);
    return "";
}

void emitStmt(std::ostringstream& os, Emitter& em, const std::shared_ptr<ir::IRStmt>& s,
              int indent) {
    if (!s) return;
    switch (s->kind) {
        case ir::IRStmt::Kind::Block:
            indentTo(os, indent);
            os << "{\n";
            em.pushScope();
            for (auto& c : s->body) emitStmt(os, em, c, indent + 1);
            em.popScope();
            indentTo(os, indent);
            os << "}\n";
            break;
        case ir::IRStmt::Kind::Declaration: {
            indentTo(os, indent);
            std::string ty = em.typeName(s->declType);
            std::string init;
            if (s->init) {
                if (s->declType.kind == ir::IRTypeKind::Array && s->declType.arraySize <= 0 &&
                    s->init->type.kind == ir::IRTypeKind::Array)
                    ty = em.typeName(s->init->type);
                init = " = " + em.expr(s->init);
            } else if (s->declType.kind == ir::IRTypeKind::Array) {
                ty = em.typeName(s->declType);
            }
            os << ty << " " << em.declareLocal(s->declName) << init << ";\n";
            break;
        }
        case ir::IRStmt::Kind::Expression:
            indentTo(os, indent);
            os << em.expr(s->expr) << ";\n";
            break;
        case ir::IRStmt::Kind::If:
            indentTo(os, indent);
            os << "if (" << em.expr(s->cond) << ") ";
            if (s->thenS && s->thenS->kind == ir::IRStmt::Kind::Block) {
                os << "\n";
                em.pushScope();
                emitStmt(os, em, s->thenS, indent);
                em.popScope();
            } else {
                os << "{\n";
                em.pushScope();
                emitStmt(os, em, s->thenS, indent + 1);
                em.popScope();
                indentTo(os, indent);
                os << "}\n";
            }
            if (s->elseS) {
                indentTo(os, indent);
                os << "else ";
                if (s->elseS->kind == ir::IRStmt::Kind::If) {
                    emitStmt(os, em, s->elseS, 0);
                } else if (s->elseS->kind == ir::IRStmt::Kind::Block) {
                    os << "\n";
                    emitStmt(os, em, s->elseS, indent);
                } else {
                    os << "{\n";
                    emitStmt(os, em, s->elseS, indent + 1);
                    indentTo(os, indent);
                    os << "}\n";
                }
            }
            break;
        case ir::IRStmt::Kind::For: {
            em.pushScope();
            indentTo(os, indent);
            os << "for (" << stmtInline(em, s->forInit) << "; " << em.expr(s->cond) << "; "
               << em.expr(s->step) << ")\n";
            if (!s->body.empty()) {
                indentTo(os, indent);
                os << "{\n";
                for (auto& c : s->body) emitStmt(os, em, c, indent + 1);
                indentTo(os, indent);
                os << "}\n";
            } else {
                indentTo(os, indent);
                os << "{\n";
                indentTo(os, indent);
                os << "}\n";
            }
            em.popScope();
            break;
        }
        case ir::IRStmt::Kind::While:
            em.pushScope();
            indentTo(os, indent);
            os << "while (" << em.expr(s->cond) << ")\n";
            indentTo(os, indent);
            os << "{\n";
            for (auto& c : s->body) emitStmt(os, em, c, indent + 1);
            indentTo(os, indent);
            os << "}\n";
            em.popScope();
            break;
        case ir::IRStmt::Kind::DoWhile:
            em.pushScope();
            indentTo(os, indent);
            os << "do\n";
            indentTo(os, indent);
            os << "{\n";
            for (auto& c : s->body) emitStmt(os, em, c, indent + 1);
            indentTo(os, indent);
            os << "} while (" << em.expr(s->cond) << ");\n";
            em.popScope();
            break;
        case ir::IRStmt::Kind::Return:
            indentTo(os, indent);
            if (s->expr) os << "return " << em.expr(s->expr) << ";\n";
            else os << "return;\n";
            break;
        case ir::IRStmt::Kind::Discard:
            indentTo(os, indent);
            os << "discard_fragment();\n";
            break;
        case ir::IRStmt::Kind::Break:
            indentTo(os, indent);
            os << "break;\n";
            break;
        case ir::IRStmt::Kind::Continue:
            indentTo(os, indent);
            os << "continue;\n";
            break;
    }
}

std::string emitHelpers(const std::set<std::string>& helpers) {
    std::ostringstream os;
    os << "inline float tglmt_mod_float(float x, float y) { return x - y * floor(x / y); }\n";
    for (int n = 2; n <= 4; ++n) {
        std::string v = "float" + std::to_string(n);
        os << "inline " << v << " tglmt_mod_" << v << "(" << v << " x, " << v
           << " y) { return x - y * floor(x / y); }\n";
        os << "inline " << v << " tglmt_mod_" << v << "(float x, " << v
           << " y) { return " << v << "(x) - y * floor(" << v << "(x) / y); }\n";
    }
    for (const auto& h : helpers) {
        if (h.rfind("tglmt_radians_", 0) == 0 || h.rfind("tglmt_degrees_", 0) == 0) {
            std::string ty = h.substr(h.rfind('_') + 1);
            bool rad = h.rfind("tglmt_radians_", 0) == 0;
            os << "inline " << ty << " " << h << "(" << ty << " x) { return x * "
               << (rad ? "0.017453292519943295" : "57.29577951308232") << "f; }\n";
            continue;
        }
        size_t p = h.find("_from_mat");
        if (p == std::string::npos) continue;
        int tc = h[9] - '0';
        int tr = h[11] - '0';
        int sc = h[21] - '0';
        int sr = h[23] - '0';
        std::string tgt = "float" + std::to_string(tc) + "x" + std::to_string(tr);
        std::string src = "float" + std::to_string(sc) + "x" + std::to_string(sr);
        os << "inline " << tgt << " " << h << "(" << src << " m) {\n";
        os << "    return " << tgt << "(";
        for (int c = 0; c < tc; ++c) {
            if (c) os << ", ";
            os << "float" << tr << "(";
            for (int r = 0; r < tr; ++r) {
                if (r) os << ", ";
                if (c < sc && r < sr) {
                    os << "m[" << c << "]." << "xyzw"[r];
                } else if (c == r) {
                    os << "1.0";
                } else {
                    os << "0.0";
                }
            }
            os << ")";
        }
        os << ");\n}\n";
    }
    return os.str();
}

bool scanUses(const std::shared_ptr<ir::IRExpr>& e, const std::string& name) {
    if (!e) return false;
    if (e->kind == ir::IRExpr::Kind::Variable && e->text == name) return true;
    for (auto& a : e->args)
        if (scanUses(a, name)) return true;
    return false;
}

bool scanStmtUses(const std::shared_ptr<ir::IRStmt>& s, const std::string& name) {
    if (!s) return false;
    if (scanUses(s->init, name) || scanUses(s->expr, name) || scanUses(s->cond, name) ||
        scanUses(s->step, name))
        return true;
    if (scanStmtUses(s->forInit, name)) return true;
    if (scanStmtUses(s->thenS, name)) return true;
    if (scanStmtUses(s->elseS, name)) return true;
    for (auto& c : s->body)
        if (scanStmtUses(c, name)) return true;
    return false;
}

} 

MSLBackend::MSLBackend() = default;
MSLBackend::~MSLBackend() = default;

bool MSLBackend::hasMetal() const {
#if defined(__APPLE__) && TARGET_OS_OSX
    std::string cmd = "xcrun -find metal >/dev/null 2>&1";
    return ::system(cmd.c_str()) == 0;
#else
    return false;
#endif
}

std::string MSLBackend::escapeMSL(const std::string& s) const {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string MSLBackend::emitMSL(const ir::IRModule& module, MSLCompileResult& out) const {
    if (module.stage == ShaderStage::Geometry || module.stage == ShaderStage::TessControl ||
        module.stage == ShaderStage::TessEval) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.code = "MTL-003";
        d.message = "MSL backend chua ho tro geometry/tessellation stage";
        out.diagnostics.push_back(d);
        out.success = false;
        return "";
    }

    Emitter em(module);

    for (const auto& u : module.uniforms) {
        em.varMap[u.name] = "tglmt_u_" + u.name;
        if (u.isAnonymousBlock && u.type.kind == ir::IRTypeKind::Struct) {
            for (const auto& f : u.type.fields)
                em.varMap[f.first] = "tglmt_u_" + u.name + "." + f.first;
        }
    }
    for (const auto& s : module.storageBlocks) em.varMap[s.name] = "tglmt_u_" + s.name;
    for (const auto& s : module.samplers) em.varMap[s.name] = "tglmt_u_" + s.name;
    for (const auto& s : module.images) em.varMap[s.name] = "tglmt_u_" + s.name;
    for (const auto& g : module.globals) {
        em.varMap[g.name] = g.name;
        if (g.storage != ir::IRStorage::Constant) em.mutableGlobals.push_back(g.name);
    }
    if (module.stage == ShaderStage::Vertex) {
        for (const auto& i : module.inputs) em.varMap[i.name] = "_in." + i.name;
    }
    for (const auto& o : module.outputs) em.varMap[o.name] = o.name;

    const ir::IRFunction* mainFn = nullptr;
    for (const auto& f : module.functions)
        if (f.isEntryPoint) mainFn = &f;

    std::ostringstream os;
    os << "#include <metal_stdlib>\n";
    os << "using namespace metal;\n\n";
    os << "constexpr sampler tglmt_linear(coord::normalized, address::repeat, "
          "filter::linear, mip_filter::linear);\n\n";

    for (const auto& st : module.structs) {
        os << "struct " << st.first << " {\n";
        for (const auto& f : st.second.fields)
            os << "    " << em.typeName(f.second) << " " << f.first << ";\n";
        os << "};\n\n";
    }

    for (const auto& g : module.globals) {
        if (g.storage != ir::IRStorage::Constant) continue;
        std::string ty = em.typeName(g.type);
        if (g.type.kind == ir::IRTypeKind::Array && g.type.arraySize <= 0 && g.init &&
            g.init->type.kind == ir::IRTypeKind::Array)
            ty = em.typeName(g.init->type);
        os << "constant " << ty << " " << g.name;
        if (g.init) os << " = " << em.expr(g.init);
        os << ";\n";
    }
    if (!module.globals.empty()) os << "\n";

    for (const auto& f : module.functions) {
        if (f.isEntryPoint) continue;
        std::string ret = em.typeName(f.returnType);
        os << ret << " " << f.name << "(";
        bool first = true;
        em.pushScope();
        for (size_t i = 0; i < f.paramTypes.size(); ++i) {
            if (!first) os << ", ";
            first = false;
            ir::IRStorage st = i < f.paramStorages.size() ? f.paramStorages[i] : ir::IRStorage::In;
            std::string pty = em.typeName(f.paramTypes[i]);
            if (st == ir::IRStorage::Out || st == ir::IRStorage::InOut) os << "thread " << pty << "& ";
            else os << pty << " ";
            os << f.paramNames[i];
            em.bindParam(f.paramNames[i]);
        }
        for (const auto& u : module.uniforms) {
            if (!first) os << ", ";
            first = false;
            os << "constant " << em.typeName(u.type) << "& tglmt_u_" << u.name;
        }
        for (const auto& s : module.storageBlocks) {
            if (!first) os << ", ";
            first = false;
            os << "constant " << em.typeName(s.type) << "& tglmt_u_" << s.name;
        }
        for (const auto& s : module.samplers) {
            if (!first) os << ", ";
            first = false;
            os << em.typeName(s.type) << " tglmt_u_" << s.name;
        }
        for (const auto& s : module.images) {
            if (!first) os << ", ";
            first = false;
            os << em.typeName(s.type) << " tglmt_u_" << s.name;
        }
        for (const auto& g : module.globals) {
            if (g.storage == ir::IRStorage::Constant) continue;
            if (!first) os << ", ";
            first = false;
            std::string gty = em.typeName(g.type);
            if (g.type.kind == ir::IRTypeKind::Array && g.type.arraySize <= 0 && g.init &&
                g.init->type.kind == ir::IRTypeKind::Array)
                gty = em.typeName(g.init->type);
            os << "thread " << gty << "& " << g.name;
        }
        os << ")\n{\n";
        for (auto& st : f.body) emitStmt(os, em, st, 1);
        em.popScope();
        os << "}\n\n";
    }

    std::vector<const ir::IRModule::IRResource*> attrInputs;
    if (module.stage == ShaderStage::Vertex) {
        for (const auto& i : module.inputs) attrInputs.push_back(&i);
    }
    bool vsUsesPointSize = false;
    for (auto& st : (mainFn ? mainFn->body : std::vector<std::shared_ptr<ir::IRStmt>>{}))
        if (scanStmtUses(st, "gl_PointSize")) vsUsesPointSize = true;

    auto bufferBinding = [&](size_t index) { return (int)index; };
    auto textureBinding = [&](size_t index) { return (int)index; };

    if (module.stage == ShaderStage::Vertex) {
        os << "struct TglmtVSOut {\n";
        os << "    float4 tglmt_gl_Position [[position]];\n";
        if (vsUsesPointSize) os << "    float tglmt_gl_PointSize [[point_size]];\n";
        for (const auto& o : module.outputs)
            os << "    " << em.typeName(o.type) << " tglmt_v_" << o.name << ";\n";
        os << "};\n\n";

        if (!attrInputs.empty()) {
            os << "struct TglmtVSIn {\n";
            int loc = 0;
            for (const auto* i : attrInputs)
                os << "    " << em.typeName(i->type) << " " << i->name << " [[attribute("
                   << loc++ << ")]];\n";
            os << "};\n\n";
        }
        os << "vertex TglmtVSOut tglmt_main_vertex(\n";
        if (!attrInputs.empty()) os << "    TglmtVSIn _in [[stage_in]],\n";
        os << "    uint tglmt_vid [[vertex_id]],\n";
        os << "    uint tglmt_iid [[instance_id]]";
        size_t bufIdx = 0, texIdx = 0;
        for (const auto& u : module.uniforms)
            os << ",\n    constant " << em.typeName(u.type) << "& tglmt_u_" << u.name
               << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
        for (const auto& s : module.storageBlocks)
            os << ",\n    constant " << em.typeName(s.type) << "& tglmt_u_" << s.name
               << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
        for (const auto& s : module.samplers) {
            if (s.type.isBufferTex)
                os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name
                   << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
            else
                os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name
                   << " [[texture(" << textureBinding(texIdx++) << ")]]";
        }
        for (const auto& s : module.images)
            os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name << " [[texture("
               << textureBinding(texIdx++) << ")]]";
        os << ")\n{\n";
        em.pushScope();
        os << "    int gl_VertexID = int(tglmt_vid);\n";
        os << "    int gl_InstanceID = int(tglmt_iid);\n";
        os << "    float4 gl_Position = float4(0.0);\n";
        if (vsUsesPointSize) os << "    float gl_PointSize = 1.0;\n";
        for (const auto& o : module.outputs) {
            std::string ty = em.typeName(o.type);
            os << "    " << ty << " " << o.name << " = " << ty << "(0);\n";
        }
        emitMutableGlobals(os, em);
        if (mainFn)
            for (auto& st : mainFn->body) emitStmt(os, em, st, 1);
        os << "\n    TglmtVSOut tglmt_out;\n";
        os << "    tglmt_out.tglmt_gl_Position = gl_Position;\n";
        if (vsUsesPointSize) os << "    tglmt_out.tglmt_gl_PointSize = gl_PointSize;\n";
        for (const auto& o : module.outputs)
            os << "    tglmt_out.tglmt_v_" << o.name << " = " << o.name << ";\n";
        em.popScope();
        os << "    return tglmt_out;\n";
        os << "}\n\n";
    } else if (module.stage == ShaderStage::Fragment) {
        os << "struct TglmtFSIn {\n";
        os << "    float4 tglmt_gl_FragCoord [[position]];\n";
        for (const auto& i : module.inputs) {
            os << "    ";
            if (i.type.isOpaque()) continue;
            os << em.typeName(i.type) << " tglmt_v_" << i.name;
            if (i.storage == ir::IRStorage::In) os << "";
            os << ";\n";
            em.varMap[i.name] = "_in.tglmt_v_" + i.name;
        }
        os << "};\n\n";

        os << "struct TglmtFSOut {\n";
        if (module.outputs.empty())
            os << "    float4 tglmt_fragColor [[color(0)]];\n";
        for (size_t i = 0; i < module.outputs.size(); ++i)
            os << "    " << em.typeName(module.outputs[i].type) << " " << module.outputs[i].name
               << " [[color(" << i << ")]];\n";
        os << "};\n\n";

        os << "fragment TglmtFSOut tglmt_main_fragment(\n";
        os << "    TglmtFSIn _in [[stage_in]],\n";
        os << "    bool gl_FrontFacing [[front_facing]],\n";
        os << "    float2 gl_PointCoord [[point_coord]]";
        size_t bufIdx = 0, texIdx = 0;
        for (const auto& u : module.uniforms)
            os << ",\n    constant " << em.typeName(u.type) << "& tglmt_u_" << u.name
               << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
        for (const auto& s : module.storageBlocks)
            os << ",\n    constant " << em.typeName(s.type) << "& tglmt_u_" << s.name
               << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
        for (const auto& s : module.samplers) {
            if (s.type.isBufferTex)
                os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name
                   << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
            else
                os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name
                   << " [[texture(" << textureBinding(texIdx++) << ")]]";
        }
        for (const auto& s : module.images)
            os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name << " [[texture("
               << textureBinding(texIdx++) << ")]]";
        os << ")\n{\n";
        em.pushScope();
        for (const auto& o : module.outputs) {
            std::string ty = em.typeName(o.type);
            os << "    " << ty << " " << o.name << " = " << ty << "(0);\n";
        }
        if (module.outputs.empty()) os << "    float4 tglmt_fragColor = float4(0.0);\n";
        emitMutableGlobals(os, em);
        if (mainFn)
            for (auto& st : mainFn->body) emitStmt(os, em, st, 1);
        os << "\n    TglmtFSOut tglmt_out;\n";
        if (module.outputs.empty()) os << "    tglmt_out.tglmt_fragColor = tglmt_fragColor;\n";
        for (const auto& o : module.outputs) os << "    tglmt_out." << o.name << " = " << o.name << ";\n";
        em.popScope();
        os << "    return tglmt_out;\n";
        os << "}\n\n";
    } else if (module.stage == ShaderStage::Compute) {
        os << "kernel void tglmt_main_compute(\n";
        os << "    uint3 gl_GlobalInvocationID [[thread_position_in_grid]],\n";
        os << "    uint3 gl_LocalInvocationID [[thread_position_in_threadgroup]],\n";
        os << "    uint3 gl_WorkGroupID [[threadgroup_position_in_grid]],\n";
        os << "    uint3 gl_NumWorkGroups [[threadgroups_per_grid]],\n";
        os << "    uint3 gl_WorkGroupSize [[threads_per_threadgroup]]";
        size_t bufIdx = 0, texIdx = 0;
        for (const auto& u : module.uniforms)
            os << ",\n    constant " << em.typeName(u.type) << "& tglmt_u_" << u.name
               << " [[buffer(" << bufferBinding(bufIdx++) << ")]]";
        for (const auto& s : module.samplers)
            os << ",\n    " << em.typeName(s.type) << " tglmt_u_" << s.name << " [[texture("
               << textureBinding(texIdx++) << ")]]";
        os << ")\n{\n";
        em.pushScope();
        emitMutableGlobals(os, em);
        if (mainFn)
            for (auto& st : mainFn->body) emitStmt(os, em, st, 1);
        em.popScope();
        os << "}\n\n";
    }

    std::string prelude = emitHelpers(em.helpers);
    size_t pos = os.str().find("constexpr sampler");
    std::string finalSrc;
    if (pos != std::string::npos) {
        size_t eol = os.str().find('\n', pos);
        finalSrc = os.str().substr(0, eol + 1) + prelude + os.str().substr(eol + 1);
    } else {
        finalSrc = os.str();
    }

    out.success = true;
    out.mslSource = finalSrc;
    return finalSrc;
}

bool MSLBackend::compileToMetallib(const std::string& mslSource,
                                   const MSLCompileOptions& opts,
                                   MSLCompileResult& out) const {
    (void)opts;
#if !defined(__APPLE__) || !TARGET_OS_OSX
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.code = "MTL-004";
    d.message = "xcrun metal chi co tren macOS; tren iOS dung runtime Metal library";
    out.diagnostics.push_back(d);
    out.success = false;
    out.mslSource = mslSource;
    return false;
#else
    if (!hasMetal()) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.code = "MTL-001";
        d.message = "xcrun metal khong ton tai tren he thong nay";
        out.diagnostics.push_back(d);
        out.success = false;
        out.mslSource = mslSource;
        return false;
    }

    static std::atomic<int> counter{0};
    std::string base = (std::filesystem::temp_directory_path() /
                        ("tglmt_" + std::to_string((long)getpid()) + "_" +
                         std::to_string(counter.fetch_add(1))))
                           .string();
    std::string srcPath = base + ".metal";
    std::string libPath = base + ".metallib";
    {
        std::ofstream f(srcPath);
        f << mslSource;
    }

    std::string cmd = "xcrun -sdk macosx metal -c " + srcPath + " -o " + libPath + " " +
                      opts.metalArgs + " 2>&1";
    FILE* pipe = ::popen(cmd.c_str(), "r");
    std::string log;
    if (pipe) {
        char buf[512];
        while (fgets(buf, sizeof(buf), pipe)) log += buf;
    }
    int rc = pipe ? ::pclose(pipe) : -1;

    if (rc != 0) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.code = "MTL-002";
        d.message = "xcrun metal that bai (rc=" + std::to_string(rc) + "): " + log;
        out.diagnostics.push_back(d);
        out.success = false;
        out.mslSource = mslSource;
        std::remove(srcPath.c_str());
        std::remove(libPath.c_str());
        return false;
    }

    std::ifstream f(libPath, std::ios::binary);
    if (f) {
        out.metallib.assign((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    }
    out.success = true;
    out.mslSource = mslSource;
    std::remove(srcPath.c_str());
    std::remove(libPath.c_str());
    return true;
#endif
}

MSLCompileResult MSLBackend::build(const ir::IRModule& module,
                                   const MSLCompileOptions& opts) const {
    MSLCompileResult out;
    emitMSL(module, out);
    if (!opts.emitSourceOnly && out.success) {
        compileToMetallib(out.mslSource, opts, out);
    }
    return out;
}

} 
