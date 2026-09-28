#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
namespace fs = std::filesystem;
using namespace tglmt; using namespace tglmt::gl;
static std::string Read(const fs::path& p){ std::ifstream f(p,std::ios::binary); std::ostringstream ss; ss<<f.rdbuf(); return ss.str(); }
int main(int argc, char** argv) {
    std::string outdir = argc > 1 ? argv[1] : "/tmp/mslcorpus";
    fs::create_directories(outdir);
    fs::path root = "tests/corpus/mc-26.1.2-shaders";
    fs::path core = root/"core", incDir = root/"include";
    Context ctx("null"); Context::MakeCurrent(&ctx);
    auto inlineImports = [&](const std::string& src){
        std::istringstream iss(src); std::string line, out; int n=0;
        while (std::getline(iss,line)) { ++n;
            std::string t=line; size_t s=t.find_first_not_of(" \t\r"); if(s!=std::string::npos) t=t.substr(s);
            if (t.rfind("#moj_import",0)==0) {
                size_t a=t.find('<'),b=t.find('>');
                std::string ref=t.substr(a+1,b-a-1); std::string name=ref.substr(ref.find(':')+1);
                out += "#line 1 0\n"+Read(incDir/name)+"\n#line "+std::to_string(n+1)+" 1\n"; continue;
            }
            out += line+"\n";
        }
        return out;
    };
    auto collectMacros = [&](const std::string& src){
        std::set<std::string> out;
        std::istringstream iss(src); std::string line;
        auto isId = [](char c, bool first){ return c=='_'||isalpha((unsigned char)c)||(!first&&isdigit((unsigned char)c)); };
        while (std::getline(iss,line)) {
            size_t s=line.find_first_not_of(" \t\r"); if(s==std::string::npos||line[s]!='#') continue;
            size_t k=s+1; while(k<line.size()&&isspace((unsigned char)line[k])) ++k;
            size_t e=k; while(e<line.size()&&isId(line[e],e==k)) ++e;
            std::string dir=line.substr(k,e-k); std::string rest=line.substr(e);
            auto addWord=[&](size_t p){ while(p<rest.size()&&!isId(rest[p],true)) ++p; size_t q=p; while(q<rest.size()&&isId(rest[q],q==p)) ++q; if(q>p) out.insert(rest.substr(p,q-p)); };
            if(dir=="ifdef"||dir=="ifndef") addWord(0);
            else if(dir=="if"||dir=="elif"){ size_t p=0; while((p=rest.find("defined",p))!=std::string::npos){ p+=7; addWord(p); } }
        }
        return out;
    };
    auto withDefines = [&](const std::string& src){
        // Blaze3D prepend defines theo program lúc runtime (không nằm trong file).
        // Dump cả base + all-on để verify_msl bao phủ mọi nhánh #ifdef (crash
        // 2026-09-28: nhánh PER_FACE_LIGHTING của entity.fsh chưa từng qua metal).
        // - PORTAL_LAYERS=16 (số lớp sao end_portal; COLORS 16 entries).
        //   Không prepend là Metal `undeclared identifier`.
        // - ALPHA_CUTOUT=0.5 (so sánh alpha); còn lại là cờ → 1.
        // Converter đã hỗ trợ #define object-like nên prepend là đủ (trung thực
        // với game, không fallback trong converter).
        std::string pre;
        // PORTAL_LAYERS là value (không nằm trong #ifdef) nên phải kiểm tra
        // dùng trực tiếp, không qua collectMacros.
        if (src.find("PORTAL_LAYERS") != std::string::npos) pre+="#define PORTAL_LAYERS 16\n";
        for (auto& m : collectMacros(src)) {
            if (m=="PORTAL_LAYERS") continue;
            else if (m=="ALPHA_CUTOUT") pre+="#define ALPHA_CUTOUT 0.5\n";
            else pre+="#define "+m+" 1\n";
        }
        return pre + src;
    };
    auto withBaseDefines = [&](const std::string& src){
        // Biến thể base: chỉ PORTAL_LAYERS (bắt buộc để compile), các macro
        // khác để tắt để lấy nhánh #else.
        if (src.find("PORTAL_LAYERS") != std::string::npos)
            return std::string("#define PORTAL_LAYERS 16\n") + src;
        return src;
    };
    std::map<std::string,GLuint> vsC, fsC;
    std::vector<std::pair<std::string,std::string>> pairs;
    for (auto& e : fs::directory_iterator(core)) {
        std::string fn = e.path().filename().string();
        if (fn.size()>4 && fn.substr(fn.size()-4)==".vsh") {
            std::string base = fn.substr(0,fn.size()-4);
            if (base=="screenquad") continue;
            if (fs::exists(core/(base+".fsh"))) pairs.emplace_back(base,base);
        }
    }
    pairs.emplace_back("screenquad","lightmap"); pairs.emplace_back("screenquad","blit_screen");
    pairs.emplace_back("animate_sprite","animate_sprite_blit"); pairs.emplace_back("animate_sprite","animate_sprite_interpolate");
    int n=0;
    auto compileVS = [&](const std::string& vn, const std::string& src)->GLuint{
        GLuint sh = glCreateShader(0x8B31); const char* p=src.c_str();
        glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
        GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) return 0;
        vsC[vn]=sh; return sh;
    };
    auto compileFS = [&](const std::string& fn, const std::string& src)->GLuint{
        GLuint sh = glCreateShader(0x8B30); const char* p=src.c_str();
        glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
        GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) return 0;
        fsC[fn]=sh; return sh;
    };
    auto dumpLink = [&](GLuint vs, GLuint fs, const std::string& tag, bool bindNormal0)->bool{
        GLuint p = glCreateProgram();
        glAttachShader(p, vs); glAttachShader(p, fs);
        if (bindNormal0) glBindAttribLocation(p, 0, "Normal"); // game bind (crumbling)
        glLinkProgram(p);
        GLint ok=0; glGetProgramiv(p,0x8B82,&ok); if(!ok) return false;
        std::ofstream(outdir+"/"+tag+".vert.metal") << ctx.programs[p].vertexMSL;
        std::ofstream(outdir+"/"+tag+".frag.metal") << ctx.programs[p].fragmentMSL;
        n+=2; return true;
    };
    for (auto& [vn,fn] : pairs) {
        std::string vsBase = withBaseDefines(inlineImports(Read(core/(vn+".vsh"))));
        std::string fsBase = withBaseDefines(inlineImports(Read(core/(fn+".fsh"))));
        std::string vsFull = withDefines(inlineImports(Read(core/(vn+".vsh"))));
        std::string fsFull = withDefines(inlineImports(Read(core/(fn+".fsh"))));
        bool hasDefines = (vsFull != vsBase) || (fsFull != fsBase);
        if (!vsC.count(vn) && !compileVS(vn, vsBase)) continue;
        if (!fsC.count(fn) && !compileFS(fn, fsBase)) continue;
        if (!dumpLink(vsC[vn], fsC[fn], vn+"__"+fn, false)) continue;
        if (hasDefines) {
            std::string vdk = vn+"+D", fdk = fn+"+D";
            if (!vsC.count(vdk) && !compileVS(vdk, vsFull)) continue;
            if (!fsC.count(fdk) && !compileFS(fdk, fsFull)) continue;
            dumpLink(vsC[vdk], fsC[fdk], vn+"__"+fn+"+defines", false);
        }
    }
    // Hồi quy crash 2026-09-28: program crumbling với bind "Normal"→0 của game.
    if (vsC.count("rendertype_crumbling") && fsC.count("rendertype_crumbling"))
        dumpLink(vsC["rendertype_crumbling"], fsC["rendertype_crumbling"],
                 "rendertype_crumbling__rendertype_crumbling.bind", true);
    // post FS + transparency VS? post FS compile-only (cả base + all-on)
    for (auto& e : fs::directory_iterator(root/"post")) {
        std::string fn = e.path().filename().string();
        if (fn.size()<5 || fn.substr(fn.size()-4)!=".fsh") continue;
        std::string base = fn.substr(0,fn.size()-4);
        std::string inlined = inlineImports(Read(e.path()));
        std::string srcBase = withBaseDefines(inlined);
        std::string srcFull = withDefines(inlined);
        auto dumpOne = [&](const std::string& src, const std::string& tag){
            GLuint sh = glCreateShader(0x8B30); const char* p=src.c_str();
            glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
            GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) return;
            auto it = ctx.shaders.find(sh);
            std::ofstream(outdir+"/"+tag) << it->second.msl;
            n+=1;
        };
        dumpOne(srcBase, "post_"+base+".frag.metal");
        if (srcFull != srcBase) dumpOne(srcFull, "post_"+base+"+defines.frag.metal");
    }
    printf("dumped %d MSL files to %s\n", n, outdir.c_str());
    return 0;
}
