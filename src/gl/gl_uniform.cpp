// gl_uniform.cpp — Uniforms/UBO/SSBO → MTLBuffer argument.
// Metal không có glUniform: TGLMT giữ shadow CPU + upload vào MTLBuffer khi draw.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/uniform_pack.h"
#include "tglmt/glsl_type.h"
#include "tglmt/limits.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>
#include <type_traits>
using namespace tglmt;
static ProgramObject* CurProg(Context& c) {
    GLuint p = 0;
    GLuint pl = 0;
    // Fast path: app không dùng program pipeline (đa số) → bỏ qua tra shadow
    // (1 phép băm hash) mỗi lệnh uniform.
    if (!c.pipelines.empty() &&
        c.state.GetShadow(0x825A /*PROGRAM_PIPELINE_BINDING*/, &pl, 4) && pl) {
        auto pit = c.pipelines.find(pl);
        if (pit != c.pipelines.end()) p = pit->second.activeProgram;
    }
    if (!p) p = c.state.BoundProgram(); // tương thích app SSO dùng glUseProgram
    if (!p) { c.errors.Record(0x0502); return nullptr; }
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
// Kiểu lưu trữ theo lệnh: lệnh *f → float, *i → int, *ui → uint, *d → double.
static constexpr uint32_t kStoreFloat = 0x1406;
static constexpr uint32_t kStoreInt = 0x1404;
static constexpr uint32_t kStoreUint = 0x1405;
static constexpr uint32_t kStoreDouble = 0x140A;
// Ghi giá trị vào slot đã tra sẵn (tránh tra hash lần 2 trên đường nóng).
// Uniform bool (bool/bvecN): GL quy đổi giá trị về TRUE/FALSE — mọi giá trị
// khác 0 thành TRUE (spec §7.6.1 cho phép nạp bằng i/ui/f) → chuẩn hoá về int
// 0/1 để đọc lại đúng.
static void StoreU(ProgramObject::UniformSlot& slot, const void* v, size_t n,
                   uint32_t storeType) {
    if (UniformTypeIsBool(slot.spec.type)) {
        const int comps = UniformComponentCount(slot.spec.type) > 0
                              ? UniformComponentCount(slot.spec.type)
                              : 1;
        const size_t per = (storeType == kStoreDouble) ? sizeof(GLdouble) : sizeof(GLfloat);
        const size_t total = n / per;
        const size_t cnt = total < (size_t)comps ? total : (size_t)comps;
        GLint b[4] = {0, 0, 0, 0};
        const uint8_t* src = (const uint8_t*)v;
        for (size_t i = 0; i < cnt && i < 4; ++i) {
            double dv = 0.0;
            if (storeType == kStoreDouble) {
                double dd;
                memcpy(&dd, src + i * per, sizeof(double));
                dv = dd;
            } else if (storeType == kStoreInt) {
                GLint ii;
                memcpy(&ii, src + i * per, sizeof(GLint));
                dv = (double)ii;
            } else if (storeType == kStoreUint) {
                GLuint uu;
                memcpy(&uu, src + i * per, sizeof(GLuint));
                dv = (double)uu;
            } else {
                float ff;
                memcpy(&ff, src + i * per, sizeof(float));
                dv = (double)ff;
            }
            b[i] = (dv != 0.0) ? 1 : 0;
        }
        slot.bytes.resize(cnt * sizeof(GLint));
        if (cnt) memcpy(slot.bytes.data(), b, cnt * sizeof(GLint));
        slot.spec.storeType = kStoreInt;
        return;
    }
    slot.bytes.resize(n);
    if (n) memcpy(slot.bytes.data(), v, n);
    slot.spec.storeType = storeType;
}
// Ghi giá trị uniform vào shadow store.
//   storeType: kiểu byte đang lưu (GL_FLOAT/GL_INT/GL_UNSIGNED_INT/GL_DOUBLE)
//   để glGetUniform* biết cách chuyển đổi khi đọc lại.
static void SetU(ProgramObject& pr, GLint loc, const void* v, size_t n, uint32_t storeType = 0) {
    // MỘT phép tra hash duy nhất (spec + bytes gộp chung trong UniformSlot).
    ProgramObject::UniformSlot& slot = pr.uniforms[loc];
    // Uniform bool (bool/bvecN): GL quy đổi giá trị về TRUE/FALSE — mọi giá
    // trị khác 0 thành TRUE. Lệnh nạp có thể là i/ui/f (spec cho phép) nhưng
    // giá trị đọc lại phải là 0/1 → chuẩn hoá về int khi lưu.
    if (UniformTypeIsBool(slot.spec.type)) {
        int comps = UniformComponentCount(slot.spec.type);
        if (comps < 1) comps = 1;
        const size_t per = (storeType == kStoreDouble) ? sizeof(GLdouble) : sizeof(GLfloat);
        const size_t total = n / per;
        const size_t cnt = total < (size_t)comps ? total : (size_t)comps;
        std::vector<GLint> b(cnt, 0);
        for (size_t i = 0; i < cnt; ++i) {
            double dv = 0.0;
            const uint8_t* src = (const uint8_t*)v + i * per;
            if (storeType == kStoreDouble) {
                double dd;
                memcpy(&dd, src, sizeof(double));
                dv = dd;
            } else if (storeType == kStoreInt) {
                GLint ii;
                memcpy(&ii, src, sizeof(GLint));
                dv = (double)ii;
            } else if (storeType == kStoreUint) {
                GLuint uu;
                memcpy(&uu, src, sizeof(GLuint));
                dv = (double)uu;
            } else {
                float ff;
                memcpy(&ff, src, sizeof(float));
                dv = (double)ff;
            }
            b[i] = (dv != 0.0) ? 1 : 0;
        }
        slot.bytes.assign((const uint8_t*)b.data(), (const uint8_t*)b.data() + b.size() * 4);
        slot.spec.storeType = kStoreInt;
        return;
    }
    // resize + memcpy: KHÔNG cấp lại vector khi kích thước không đổi (đường
    // nóng: app set lại cùng uniform mỗi frame).
    slot.bytes.resize(n);
    if (n) memcpy(slot.bytes.data(), v, n);
    if (storeType) slot.spec.storeType = storeType;
}
// GL 4.6 §7.6.1: "An INVALID_VALUE error is generated if count is less than
// zero." Trước đây count < 0 → (size_t)n khổng lồ → std::length_error →
// std::terminate (abort, KHÔNG bắt được trong hầu hết host). Ngoài ra
// location = -1 (không có uniform tại location) hoặc location không tồn tại
// → INVALID_OPERATION.
static bool UniformCountOk(Context& c, GLsizei n, const void* v) {
    if (n < 0) { c.errors.Record(0x0501); return false; }
    if (n > 0 && !v) { c.errors.Record(0x0501); return false; }
    return true;
}
// Diag tint (1.21): ColorModulator/FogColor là standalone uniform — log giá trị
// game set (dedupe theo prog|name|value, tối đa 400 dòng) để đối chiếu bytes
// upload (tintdraw trong gl_apple_draw) khi TGLMT_DIAG=1.
static void DiagUniform4(ProgramObject& pr, GLint loc, const float* v) {
    Context& c = Context::Current();
    if (!c.DiagOn()) return;
    for (auto& kv : pr.uniformLoc) {
        if (kv.second != loc) continue;
        const std::string& nm = kv.first;
        if (nm != "ColorModulator" && nm != "FogColor") return;
        static std::set<std::string> seen;
        char key[160];
        snprintf(key, sizeof key, "%u|%s|%.4f,%.4f,%.4f,%.4f", pr.id, nm.c_str(), v[0], v[1],
                 v[2], v[3]);
        if (!seen.insert(key).second || seen.size() > 400) return;
        fprintf(stderr, "[TGLMT] uni4f prog=%u %s = %.4f %.4f %.4f %.4f\n", pr.id, nm.c_str(),
                v[0], v[1], v[2], v[3]);
        fflush(stderr);
        return;
    }
}
// Sampler unit tracking: glUniform1i(samplerLoc, unit) → samplerUnits[name]=unit.
// Đúng GL: sampler uniform giá trị là texture unit. AppleDrawGL bind slot k theo unit này.
static void TrackSamplerUnit(ProgramObject& pr, GLint loc, GLint unit) {
    if (unit < 0 || unit >= 32) return;
    for (auto& kv : pr.uniformLoc) {
        if (kv.second != loc) continue;
        const std::string nm = kv.first;
        auto isSamp = [&](const std::vector<std::string>& lst) {
            for (auto& s : lst) if (s == nm) return true;
            return false;
        };
        auto isStageSamp = [&](const std::vector<ProgramObject::StageSampler>& lst) {
            for (auto& x : lst) if (x.name == nm) return true;
            return false;
        };
        auto isImg = [&](const std::vector<ProgramObject::ImageSlot>& lst) {
            for (auto& x : lst) if (x.name == nm) return true;
            return false;
        };
        if (isSamp(pr.vsSamplers) || isSamp(pr.fsSamplers) ||
            isStageSamp(pr.vsCaptureSamplers) || isStageSamp(pr.tcsSamplers) ||
            isStageSamp(pr.tesSamplers) || isStageSamp(pr.fsTessSamplers) ||
            isImg(pr.tcsImages) || isImg(pr.tesImages)) {
            pr.samplerUnits[nm] = (GLuint)unit;
            return;
        }
        // program chưa link / sampler list rỗng (link cũ): thử tên trong conv? bỏ qua
        return;
    }
}

// ===========================================================================
// GL 4.6 §7.6.1 — kiểm tra lệnh uniform TRƯỚC khi ghi giá trị.
//
// Trước đây TGLMT ghi thẳng vào shadow store: sai kiểu/sai số phần tử/không
// tồn tại location đều bị nuốt im lặng, và location == -1 (spec: "silently
// ignore") vẫn bị ghi vào store. Đây là sai lệch thật so với spec — GL phải
// sinh INVALID_OPERATION/INVALID_VALUE theo đúng điều kiện dưới đây.
//
//   INVALID_VALUE  : count < 0
//   INVALID_VALUE  : Uniform1i{v} nạp sampler ngoài [0, MAX_COMBINED_TEXTURE_IMAGE_UNITS)
//   INVALID_VALUE  : Uniform1i{v} nạp image ngoài [0, MAX_IMAGE_UNITS)
//   INVALID_OPERATION: kích thước trong tên lệnh ≠ kích thước uniform khai
//   INVALID_OPERATION: kiểu thành phần ≠ kiểu uniform (trừ bool: bool khớp
//                     MỌI lệnh i/ui/f)
//   INVALID_OPERATION: count > 1 mà uniform không phải mảng
//   INVALID_OPERATION: không có biến nào ở location đó (location != -1)
//   INVALID_OPERATION: nạp sampler/image bằng lệnh khác Uniform1i{v}
//   location == -1  : bỏ qua IM LẶNG, không đổi giá trị, KHÔNG sinh lỗi
// ===========================================================================
// Mô tả lệnh uniform đang chạy (dùng để đối chiếu với kiểu uniform).
struct UniformCmd {
    uint32_t type = 0;   // GL_FLOAT / GL_INT / GL_UNSIGNED_INT / GL_DOUBLE
    int size = 0;        // số thành phần (1..4)
    int cols = 0;        // ma trận: số cột (0 = không phải lệnh ma trận)
    int rows = 0;        // ma trận: số hàng
};
// Ghi log kiểu uniform để chẩn đoán khi validation fail.
static const char* UniformTypeName(uint32_t t) {
    switch (t) {
        case 0x1406: return "GL_FLOAT";
        case 0x1404: return "GL_INT";
        case 0x1405: return "GL_UNSIGNED_INT";
        case 0x140A: return "GL_DOUBLE";
        case 0x8B56: return "GL_BOOL";
        case 0x8B50: return "GL_FLOAT_VEC2";
        case 0x8B51: return "GL_FLOAT_VEC3";
        case 0x8B52: return "GL_FLOAT_VEC4";
        case 0x8B53: return "GL_INT_VEC2";
        case 0x8B54: return "GL_INT_VEC3";
        case 0x8B55: return "GL_INT_VEC4";
        case 0x8DC6: return "GL_UNSIGNED_INT_VEC2";
        case 0x8DC7: return "GL_UNSIGNED_INT_VEC3";
        case 0x8DC8: return "GL_UNSIGNED_INT_VEC4";
        case 0x8B57: return "GL_BOOL_VEC2";
        case 0x8B58: return "GL_BOOL_VEC3";
        case 0x8B59: return "GL_BOOL_VEC4";
        case 0x8B5A: return "GL_FLOAT_MAT2";
        case 0x8B5B: return "GL_FLOAT_MAT3";
        case 0x8B5C: return "GL_FLOAT_MAT4";
        case 0x8B65: return "GL_FLOAT_MAT2x3";
        case 0x8B66: return "GL_FLOAT_MAT2x4";
        case 0x8B67: return "GL_FLOAT_MAT3x2";
        case 0x8B68: return "GL_FLOAT_MAT3x4";
        case 0x8B69: return "GL_FLOAT_MAT4x2";
        case 0x8B6A: return "GL_FLOAT_MAT4x3";
        default: return "GL_<unknown>";
    }
}
// Kiểm tra lệnh uniform theo §7.6.1.
//   ivals: con trỏ tới giá trị kiểu int (dùng cho kiểm tra range của sampler/
//          image). nullptr ⇒ không có giá trị int nào để kiểm tra.
// Trả về true nghĩa là lệnh hợp lệ (được ghi). false nghĩa là phải bỏ qua
// (đã sinh lỗi theo spec, hoặc location == -1 → bỏ qua im lặng).
// Trả về slot của uniform nếu lệnh hợp lệ (đã validate theo §7.6.1), nullptr
// nếu lệnh bị từ chối (đã sinh lỗi) hoặc location == -1 (bỏ qua im lặng).
// MỘT phép tra hash cho cả validate LẪN ghi — trước đây là hai (spec + bytes).
static ProgramObject::UniformSlot* UniformSlotOk(Context& c, ProgramObject& pr, GLint loc,
                                                const UniformCmd& cmd, GLsizei count,
                                                const GLint* ivals) {
    // Spec: "If the value of location is -1, the Uniform* commands will silently
    // ignore the data passed in, and the current uniform values will not be
    // changed." — KHÔNG sinh lỗi.
    if (loc == -1) return nullptr;
    auto sit = pr.uniforms.find(loc);
    if (sit == pr.uniforms.end()) {
        // "no variable with a location of location exists in the program object
        // currently in use and location is not -1" → INVALID_OPERATION.
        c.errors.Record(0x0502);
        return nullptr;
    }
    const ProgramObject::UniformSlot::Spec& sp = sit->second.spec;
    // --- Sampler / image: chỉ Uniform1i{v} mới nạp được ---
    if (sp.isSampler || sp.isImage) {
        const bool isInt1 = (cmd.type == 0x1404 && cmd.size == 1 && cmd.cols == 0);
        if (!isInt1) {
            // "a sampler or image uniform is loaded with any of the Uniform*
            // commands other than Uniform1i{v}" → INVALID_OPERATION.
            c.errors.Record(0x0502);
            return nullptr;
        }
        const int limit = sp.isImage ? kMaxImageUnits : kMaxCombinedTextureImageUnits;
        for (GLsizei i = 0; i < count; ++i) {
            const GLint v = ivals ? ivals[i] : 0;
            if (v < 0 || v >= limit) {
                // "An INVALID_VALUE error is generated if Uniform1i{v} is used
                // to set a sampler/image uniform to a value less than zero or
                // greater than or equal to the value of MAX_...".
                c.errors.Record(0x0501);
                return nullptr;
            }
        }
        return &sit->second;
    }
    // --- Kiểu / kích thước ---
    if (sp.type != 0) {  // 0 = không xác định được kiểu → permissive
        const int ucols = 0, urows = 0;
        int mc = 0, mr = 0;
        const bool uniformIsMatrix = UniformTypeIsMatrix(sp.type, mc, mr);
        if (uniformIsMatrix) {
            // Uniform là ma trận ⇒ lệnh phải là lệnh ma trận CÙNG kích thước.
            if (cmd.cols == 0 || cmd.cols != mc || cmd.rows != mr || cmd.type != 0x1406) {
                c.errors.Record(0x0502);
                return nullptr;
            }
        } else {
            // Uniform không phải ma trận ⇒ lệnh ma trận không khớp.
            if (cmd.cols != 0) {
                c.errors.Record(0x0502);
                return nullptr;
            }
            if (UniformTypeIsBool(sp.type)) {
                // "a boolean uniform component type is considered to match any
                // of the Uniform*i{v}, Uniform*ui{v}, or Uniform*f{v} commands"
                // → double KHÔNG khớp bool.
                if (cmd.type == 0x140A) {
                    c.errors.Record(0x0502);
                    return nullptr;
                }
            } else if (UniformBaseType(sp.type) != cmd.type) {
                c.errors.Record(0x0502);
                return nullptr;
            }
            const int comps = UniformComponentCount(sp.type);
            if (comps && comps != cmd.size) {
                c.errors.Record(0x0502);
                return nullptr;
            }
        }
        (void)ucols;
        (void)urows;
    }
    // --- count > 1 đòi uniform phải là mảng ---
    if (count > 1 && sp.arraySize <= 1) {
        // "count is greater than one, and the uniform declared in the shader is
        // not an array variable" → INVALID_OPERATION.
        c.errors.Record(0x0502);
        return nullptr;
    }
    return &sit->second;
}
// Lấy program theo id cho đường DSA (glProgramUniform*).
//   GL 4.6 §7.6.1: "An INVALID_VALUE error is generated if program is not the
//   name of either a program or shader object." + "An INVALID_OPERATION error
//   is generated if program is the name of a shader object." + "An
//   INVALID_OPERATION error is generated if program has not been linked
//   successfully."
// Trả về nullptr kèm lỗi đã ghi.
static ProgramObject* ProgramForUniform(Context& c, GLuint p) {
    auto it = c.programs.find(p);
    if (it != c.programs.end()) {
        if (!it->second.linked) {
            c.errors.Record(0x0502);  // chưa link → INVALID_OPERATION
            return nullptr;
        }
        return &it->second;
    }
    if (c.shaders.count(p)) {
        c.errors.Record(0x0502);  // là shader object → INVALID_OPERATION
        return nullptr;
    }
    c.errors.Record(0x0501);  // không phải program/shader → INVALID_VALUE
    return nullptr;
}
namespace tglmt::gl {
// Tên có tồn tại trong reflection của program? (link đã điền đầy đủ các list).
// Gồm: sampler vs/fs/compute, uniform thường (uniformLayout), image, atomic
// counter — tất cả đều nhận location hợp lệ trong GL.
static bool ProgramHasUniformName(const ProgramObject& pr, const std::string& nm) {
    auto isIn = [&](const std::vector<std::string>& lst) {
        for (auto& s : lst) if (s == nm) return true;
        return false;
    };
    if (isIn(pr.vsSamplers) || isIn(pr.fsSamplers) || isIn(pr.computeSamplers)) return true;
    auto inStageSamp = [&](const std::vector<ProgramObject::StageSampler>& lst) {
        for (auto& s : lst) if (s.name == nm) return true;
        return false;
    };
    if (inStageSamp(pr.vsCaptureSamplers) || inStageSamp(pr.tcsSamplers) ||
        inStageSamp(pr.tesSamplers) || inStageSamp(pr.fsTessSamplers)) return true;
    for (auto& e : pr.uniformLayout) if (e.name == nm) return true;
    auto inImg = [&](const std::vector<ProgramObject::ImageSlot>& v) {
        for (auto& im : v) if (im.name == nm) return true;
        return false;
    };
    if (inImg(pr.vsImages) || inImg(pr.fsImages) || inImg(pr.computeImages) ||
        inImg(pr.tcsImages) || inImg(pr.tesImages)) return true;
    auto inAtom = [&](const std::vector<ProgramObject::AtomicSlot>& v) {
        for (auto& a : v) if (a.name == nm) return true;
        return false;
    };
    if (inAtom(pr.vsAtomics) || inAtom(pr.fsAtomics) || inAtom(pr.computeAtomics)) return true;
    return false;
}
GLint glGetUniformLocation(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return -1; }
    auto& pr = it->second;
    std::string nm = n ? n : "";
    // GL cho query "foo" hoặc "foo[0]" (phần tử mảng) — strip hậu tố "[số]"
    // (converter lưu tên mảng đã strip → tra theo base name).
    if (!nm.empty() && nm.back() == ']') {
        size_t lb = nm.rfind('[');
        if (lb != std::string::npos && lb + 1 < nm.size()) {
            bool digits = true;
            for (size_t i = lb + 1; i + 1 < nm.size(); ++i)
                if (!isdigit((unsigned char)nm[i])) { digits = false; break; }
            if (digits) nm = nm.substr(0, lb);
        }
    }
    // Program ĐÃ link + reflection đầy đủ → trung thực theo GL: tên không có
    // trong program (chưa khai báo, hoặc bị converter prune vì không dùng) → -1.
    // Bắt buộc cho Iris: nó dùng location != -1 để đếm texture unit; nếu mọi
    // tên đều trả loc thì composite hết unit khi tới dhDepthTex → pipeline fail.
    // Chưa link (hoặc reflection chưa điền) → giữ nới lỏng có chủ đích để app
    // cache loc sớm — loc vẫn đúng sau link vì tra theo tên.
    if (pr.linked && pr.reflectionComplete && !ProgramHasUniformName(pr, nm)) return -1;
    auto f = pr.uniformLoc.find(nm);
    if (f != pr.uniformLoc.end()) {
        // Loc cache sớm (trước link) / query lại: PHẢI điền e.loc cho
        // uniformLayout — AppleDraw upload theo (loc >= 0), thiếu là uniform
        // không bao giờ tới shader (FS đọc 0 → đen/tint sai).
        // Ghi cho MỌI entry cùng tên (uniform khai ở cả VS và FS = 2 bản sao,
        // cùng location) — chỉ ghi entry đầu sẽ để stage còn lại loc=-1.
        for (auto& e : pr.uniformLayout)
            if (e.name == nm) { e.loc = f->second; }
        return f->second;
    }
    // Gán location mới: phải TRÁNH location đã dùng (explicit location theo
    // ARB_explicit_uniform_location có thể rải rác → uniformLoc.size() không
    // còn là "max+1"). Duyệt từ max+1, bỏ qua location đã chiếm.
    GLint loc = 0;
    for (auto& kv : pr.uniformLoc) loc = std::max(loc, kv.second + 1);
    while ([&] {
        for (auto& kv : pr.uniformLoc)
            if (kv.second == loc) return true;
        return false;
    }()) ++loc;
    pr.uniformLoc[nm] = loc;
    for (auto& e : pr.uniformLayout)
        if (e.name == nm) { e.loc = loc; }
    // Đăng ký spec cho location MỚI (vd sampler/image chỉ có location sau khi
    // app hỏi) — nếu bỏ sót, lệnh glUniform* sau đó sẽ bị từ chối oan.
    {
        ProgramObject::UniformSlot::Spec sp;
        for (const auto& au : pr.activeUniforms)
            if (au.name == nm || au.name == nm + "[0]") {
                sp.type = au.type;
                sp.arraySize = au.size > 0 ? au.size : 1;
                break;
            }
        if (!sp.type)
            for (const auto& e : pr.uniformLayout)
                if (e.name == nm) {
                    sp.type = e.glType;
                    sp.arraySize = e.arraySize > 0 ? e.arraySize : 1;
                    break;
                }
        sp.isSampler = UniformTypeIsSampler(sp.type);
        sp.isImage = UniformTypeIsImage(sp.type);
        pr.uniforms[loc].spec = sp;
    }
    return loc;
}
GLuint glGetUniformBlockIndex(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !n) { c.errors.Record(0x0502); return 0xFFFFFFFFu; }
    for (auto& b : it->second.uniformBlocks)
        if (b.name == n) return b.index;
    return 0xFFFFFFFFu; // GL_INVALID_INDEX khi block không tồn tại (đúng GL)
}
void glUniformBlockBinding(GLuint p, GLuint b, GLuint bi) {
    Context& c = Context::Current();
    {
        // Chẩn đoán misbound UBO: chỉ khi TGLMT_DIAG=1.
        if (c.DiagOn()) {
            static int nBind = 0;
            if (++nBind <= 400) {
                std::string nm = "?";
                auto it0 = c.programs.find(p);
                if (it0 != c.programs.end())
                    for (auto& blk : it0->second.uniformBlocks)
                        if (blk.index == b) { nm = blk.name; break; }
                fprintf(stderr, "[TGLMT] ublockbind#%d prog@%u idx=%u(%s) -> point %u\n",
                        nBind, p, b, nm.c_str(), bi);
                fflush(stderr);
            }
        }
    }
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    for (auto& blk : it->second.uniformBlocks)
        if (blk.index == b) { blk.binding = bi; return; }
    // block index chưa có (gọi trước link): link sẽ giữ binding từ lần trước
    // hoặc layout(binding=N); log sang stderr vì LogDebug câm trên iOS (không debugCb).
    if (c.DiagOn()) {
        static int nMiss = 0;
        if (++nMiss <= 64)
            fprintf(stderr, "[TGLMT] ublockmiss#%d prog@%u idx=%u point=%u (chưa link, giữ layout/0)\n",
                    nMiss, p, b, bi);
        fflush(stderr);
    }
    (void)bi;
}
// P4: đổi binding point của shader storage block — override layout(binding=N).
// storageBlockIndex = thứ tự block `buffer` program-scope (vs rồi fs, dedupe tên).
// Áp ở bind time (draw + compute) qua ProgramObject::EffectiveSsboBinding.
void glShaderStorageBlockBinding(GLuint p, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !it->second.linked) { c.errors.Record(0x0501); return; }
    if (storageBlockIndex >= it->second.storageBlocks.size()) { c.errors.Record(0x0501); return; }
    it->second.ssboBindOverride[it->second.storageBlocks[storageBlockIndex].name] = storageBlockBinding;
}
void glUniformSubroutinesuiv(GLenum a, GLsizei b, const GLuint* c_) {
    Context& c = Context::Current();
    if (!(a == 0x8B31 || a == 0x8B30 || a == 0x8DD9 || a == 0x8E88 || a == 0x8E87)) {
        c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    auto si = pr->subroutines.find(a);
    if (si == pr->subroutines.end()) { c.errors.Record(0x0502); return; }
    auto& info = si->second;
    if (b != (GLsizei)info.uniforms.size() || (b > 0 && !c_)) {
        c.errors.Record(0x0501); return; // count phải bằng số active subroutine uniform
    }
    // Validate toàn bộ trước khi ghi (tránh nửa state khi index sai).
    std::vector<GLint> ordinals((size_t)b);
    for (GLsizei i = 0; i < b; ++i) {
        if (c_[i] >= info.impls.size()) { c.errors.Record(0x0501); return; }
        auto& ty = info.uniforms[(size_t)i].second;
        auto ti = info.typeImpls.find(ty);
        if (ti == info.typeImpls.end()) { c.errors.Record(0x0502); return; }
        const std::string& fn = info.impls[c_[i]];
        int ord = -1;
        for (size_t k = 0; k < ti->second.size(); ++k)
            if (ti->second[k] == fn) { ord = (int)k; break; }
        if (ord < 0) { c.errors.Record(0x0502); return; } // index khác type
        ordinals[(size_t)i] = ord;
    }
    for (GLsizei i = 0; i < b; ++i) {
        auto li = pr->uniformLoc.find(info.uniforms[(size_t)i].first);
        if (li == pr->uniformLoc.end()) continue;
        pr->uniforms[li->second].bytes.assign((const uint8_t*)&ordinals[(size_t)i],
                                              (const uint8_t*)&ordinals[(size_t)i] + 4);
    }
}
// ============ glUniform* (đường non-DSA: program đang dùng) ============
void glUniform1f(GLint l, GLfloat a0) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLfloat v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 1, kStoreFloat);
}

void glUniform1i(GLint l, GLint a0) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLint v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 1, 0, 0}, 1, v);
    if (!slot) return;
    TrackSamplerUnit(*pr, l, a0);
    StoreU(*slot, v, sizeof(GLint) * 1, kStoreInt);
}

void glUniform1ui(GLint l, GLuint a0) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLuint v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 1, kStoreUint);
}

void glUniform1d(GLint l, GLdouble a0) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLdouble v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 1, kStoreDouble);
}

void glUniform2f(GLint l, GLfloat a0, GLfloat a1) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLfloat v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 2, kStoreFloat);
}

void glUniform2i(GLint l, GLint a0, GLint a1) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLint v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 2, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 2, kStoreInt);
}

void glUniform2ui(GLint l, GLuint a0, GLuint a1) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLuint v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 2, kStoreUint);
}

void glUniform2d(GLint l, GLdouble a0, GLdouble a1) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLdouble v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 2, kStoreDouble);
}

void glUniform3f(GLint l, GLfloat a0, GLfloat a1, GLfloat a2) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLfloat v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 3, kStoreFloat);
}

void glUniform3i(GLint l, GLint a0, GLint a1, GLint a2) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLint v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 3, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 3, kStoreInt);
}

void glUniform3ui(GLint l, GLuint a0, GLuint a1, GLuint a2) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLuint v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 3, kStoreUint);
}

void glUniform3d(GLint l, GLdouble a0, GLdouble a1, GLdouble a2) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLdouble v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 3, kStoreDouble);
}

void glUniform4f(GLint l, GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLfloat v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 4, kStoreFloat);
    DiagUniform4(*pr, l, v);
}

void glUniform4i(GLint l, GLint a0, GLint a1, GLint a2, GLint a3) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLint v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 4, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 4, kStoreInt);
}

void glUniform4ui(GLint l, GLuint a0, GLuint a1, GLuint a2, GLuint a3) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLuint v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 4, kStoreUint);
}

void glUniform4d(GLint l, GLdouble a0, GLdouble a1, GLdouble a2, GLdouble a3) {
    Context& c = Context::Current();
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    const GLdouble v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 4, kStoreDouble);
}

void glUniform1fv(GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 1, kStoreFloat);
}

void glUniform1iv(GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 1, 0, 0}, n, v);
    if (!slot) return;
    if (n > 0) TrackSamplerUnit(*pr, l, v[0]);
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 1, kStoreInt);
}

void glUniform1uiv(GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 1, kStoreUint);
}

void glUniform1dv(GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 1, kStoreDouble);
}

void glUniform2fv(GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 2, kStoreFloat);
}

void glUniform2iv(GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 2, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 2, kStoreInt);
}

void glUniform2uiv(GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 2, kStoreUint);
}

void glUniform2dv(GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 2, kStoreDouble);
}

void glUniform3fv(GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 3, kStoreFloat);
}

void glUniform3iv(GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 3, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 3, kStoreInt);
}

void glUniform3uiv(GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 3, kStoreUint);
}

void glUniform3dv(GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 3, kStoreDouble);
}

void glUniform4fv(GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 4, kStoreFloat);
    if (n > 0) DiagUniform4(*pr, l, v);
}

void glUniform4iv(GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 4, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 4, kStoreInt);
}

void glUniform4uiv(GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 4, kStoreUint);
}

void glUniform4dv(GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 4, kStoreDouble);
}

void glUniformMatrix2fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix2x3fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix2x4fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix3fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix3x2fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix3x4fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix4fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix4x2fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix4x3fv(GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glUniformMatrix2dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix2x3dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix2x4dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix3dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix3x2dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix3x4dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix4dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix4x2dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glUniformMatrix4x3dv(GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = CurProg(c);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}


// ====== glProgramUniform* (đường DSA: program theo id, §7.6.1) ======
void glProgramUniform1f(GLuint p, GLint l, GLfloat a0) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLfloat v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 1, kStoreFloat);
}

void glProgramUniform1i(GLuint p, GLint l, GLint a0) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLint v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 1, 0, 0}, 1, v);
    if (!slot) return;
    TrackSamplerUnit(*pr, l, a0);
    StoreU(*slot, v, sizeof(GLint) * 1, kStoreInt);
}

void glProgramUniform1ui(GLuint p, GLint l, GLuint a0) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLuint v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 1, kStoreUint);
}

void glProgramUniform1d(GLuint p, GLint l, GLdouble a0) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLdouble v[1] = {a0};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 1, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 1, kStoreDouble);
}

void glProgramUniform2f(GLuint p, GLint l, GLfloat a0, GLfloat a1) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLfloat v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 2, kStoreFloat);
}

void glProgramUniform2i(GLuint p, GLint l, GLint a0, GLint a1) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLint v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 2, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 2, kStoreInt);
}

void glProgramUniform2ui(GLuint p, GLint l, GLuint a0, GLuint a1) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLuint v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 2, kStoreUint);
}

void glProgramUniform2d(GLuint p, GLint l, GLdouble a0, GLdouble a1) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLdouble v[2] = {a0, a1};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 2, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 2, kStoreDouble);
}

void glProgramUniform3f(GLuint p, GLint l, GLfloat a0, GLfloat a1, GLfloat a2) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLfloat v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 3, kStoreFloat);
}

void glProgramUniform3i(GLuint p, GLint l, GLint a0, GLint a1, GLint a2) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLint v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 3, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 3, kStoreInt);
}

void glProgramUniform3ui(GLuint p, GLint l, GLuint a0, GLuint a1, GLuint a2) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLuint v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 3, kStoreUint);
}

void glProgramUniform3d(GLuint p, GLint l, GLdouble a0, GLdouble a1, GLdouble a2) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLdouble v[3] = {a0, a1, a2};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 3, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 3, kStoreDouble);
}

void glProgramUniform4f(GLuint p, GLint l, GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLfloat v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * 4, kStoreFloat);
    DiagUniform4(*pr, l, v);
}

void glProgramUniform4i(GLuint p, GLint l, GLint a0, GLint a1, GLint a2, GLint a3) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLint v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 4, 0, 0}, 1, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * 4, kStoreInt);
}

void glProgramUniform4ui(GLuint p, GLint l, GLuint a0, GLuint a1, GLuint a2, GLuint a3) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLuint v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * 4, kStoreUint);
}

void glProgramUniform4d(GLuint p, GLint l, GLdouble a0, GLdouble a1, GLdouble a2, GLdouble a3) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    const GLdouble v[4] = {a0, a1, a2, a3};
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 4, 0, 0}, 1, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * 4, kStoreDouble);
}

void glProgramUniform1fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 1, kStoreFloat);
}

void glProgramUniform1iv(GLuint p, GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 1, 0, 0}, n, v);
    if (!slot) return;
    if (n > 0) TrackSamplerUnit(*pr, l, v[0]);
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 1, kStoreInt);
}

void glProgramUniform1uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 1, kStoreUint);
}

void glProgramUniform1dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 1, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 1, kStoreDouble);
}

void glProgramUniform2fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 2, kStoreFloat);
}

void glProgramUniform2iv(GLuint p, GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 2, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 2, kStoreInt);
}

void glProgramUniform2uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 2, kStoreUint);
}

void glProgramUniform2dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 2, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 2, kStoreDouble);
}

void glProgramUniform3fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 3, kStoreFloat);
}

void glProgramUniform3iv(GLuint p, GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 3, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 3, kStoreInt);
}

void glProgramUniform3uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 3, kStoreUint);
}

void glProgramUniform3dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 3, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 3, kStoreDouble);
}

void glProgramUniform4fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLfloat) * (size_t)n * 4, kStoreFloat);
    if (n > 0) DiagUniform4(*pr, l, v);
}

void glProgramUniform4iv(GLuint p, GLint l, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1404, 4, 0, 0}, n, v);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLint) * (size_t)n * 4, kStoreInt);
}

void glProgramUniform4uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1405, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLuint) * (size_t)n * 4, kStoreUint);
}

void glProgramUniform4dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 4, 0, 0}, n, nullptr);
    if (!slot) return;
    StoreU(*slot, v, sizeof(GLdouble) * (size_t)n * 4, kStoreDouble);
}

void glProgramUniformMatrix2fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix2x3fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix2x4fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 2, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 2, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix3fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix3x2fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix3x4fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 3, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 3, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix4fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)4;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 4);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix4x2fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)2;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 2);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix4x3fv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLfloat* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x1406, 0, 4, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)3;
    const size_t tight = comps * sizeof(GLfloat);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i)
                TransposeGlslMatrix(v + (size_t)i * comps,
                                    tmp.data() + (size_t)i * tight, 4, 3);
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreFloat);
}

void glProgramUniformMatrix2dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix2x3dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix2x4dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 2, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)2 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 2; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)2 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix3dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix3x2dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix3x4dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 3, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)3 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 3; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)3 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix4dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 4}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)4;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 4; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)4 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix4x2dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 2}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)2;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 2; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)2 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

void glProgramUniformMatrix4x3dv(GLuint p, GLint l, GLsizei n, GLboolean transpose, const GLdouble* v) {
    Context& c = Context::Current();
    if (!UniformCountOk(c, n, v)) return;
    ProgramObject* pr = ProgramForUniform(c, p);
    if (!pr) return;
    ProgramObject::UniformSlot* slot = UniformSlotOk(c, *pr, l, UniformCmd{0x140A, 0, 4, 3}, n, nullptr);
    if (!slot) return;
    const size_t comps = (size_t)4 * (size_t)3;
    const size_t tight = comps * sizeof(GLdouble);
    std::vector<uint8_t> tmp((size_t)n * tight);
    if (n > 0) {
        if (transpose) {
            for (GLsizei i = 0; i < n; ++i) {
                const GLdouble* s = v + (size_t)i * comps;
                GLdouble* d = (GLdouble*)(tmp.data() + (size_t)i * tight);
                for (int r = 0; r < 3; ++r)
                    for (int cc = 0; cc < 4; ++cc)
                        d[(size_t)cc * (size_t)3 + (size_t)r] =
                            s[(size_t)r * (size_t)4 + (size_t)cc];
            }
        } else {
            memcpy(tmp.data(), v, (size_t)n * tight);
        }
    }
    StoreU(*slot, tmp.data(), (size_t)n * tight, kStoreDouble);
}

} // namespace tglmt::gl
