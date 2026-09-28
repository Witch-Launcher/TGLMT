#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
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
    auto withDefines = [&](const std::string& src){
        // Blaze3D prepend defines theo program lúc runtime (không nằm trong file).
        // - PORTAL_LAYERS: số lớp sao end_portal/gateway (uniform cũ đã thay bằng
        //   define từ 25w10a; COLORS có 16 entries nên end_portal = 16).
        //   Không prepend là Metal `undeclared identifier` (đã quan sát).
        //   Converter đã hỗ trợ #define object-like nên prepend là đủ, không cần
        //   fallback trong converter (trung thực với game).
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
    for (auto& [vn,fn] : pairs) {
        if (!vsC.count(vn)) {
            std::string src = withDefines(inlineImports(Read(core/(vn+".vsh"))));
            GLuint sh = glCreateShader(0x8B31); const char* p=src.c_str();
            glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
            GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) continue;
            vsC[vn]=sh;
        }
        if (!fsC.count(fn)) {
            std::string src = withDefines(inlineImports(Read(core/(fn+".fsh"))));
            GLuint sh = glCreateShader(0x8B30); const char* p=src.c_str();
            glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
            GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) continue;
            fsC[fn]=sh;
        }
        GLuint p = glCreateProgram();
        glAttachShader(p, vsC[vn]); glAttachShader(p, fsC[fn]); glLinkProgram(p);
        GLint ok=0; glGetProgramiv(p,0x8B82,&ok); if(!ok) continue;
        std::string tag = vn+"__"+fn;
        std::ofstream(outdir+"/"+tag+".vert.metal") << ctx.programs[p].vertexMSL;
        std::ofstream(outdir+"/"+tag+".frag.metal") << ctx.programs[p].fragmentMSL;
        ++n;
    }
    // post FS + transparency VS? post FS compile-only
    for (auto& e : fs::directory_iterator(root/"post")) {
        std::string fn = e.path().filename().string();
        if (fn.size()<5 || fn.substr(fn.size()-4)!=".fsh") continue;
        std::string base = fn.substr(0,fn.size()-4);
        std::string src = withDefines(inlineImports(Read(e.path())));
        GLuint sh = glCreateShader(0x8B30); const char* p=src.c_str();
        glShaderSource(sh,1,&p,nullptr); glCompileShader(sh);
        GLint ok=0; glGetShaderiv(sh,0x8B81,&ok); if(!ok) continue;
        auto it = ctx.shaders.find(sh);
        std::ofstream(outdir+"/post_"+base+".frag.metal") << it->second.msl;
        ++n;
    }
    printf("dumped %d MSL files to %s\n", n*2, outdir.c_str());
    return 0;
}
