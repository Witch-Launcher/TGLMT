// aquarium.cpp — toàn bộ scene/vật lý/render bằng tglmt::gl (OpenGL 4.6 core).
// Xem aquarium.h. Shader: apps/aquarium/shaders/*.vert|*.frag (subset converter).
#include "aquarium.h"
#include "tglmt/GLAppleDraw.h"
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace aquarium {
namespace gl = tglmt::gl;

// ---------- toán mat4 column-major (chuẩn GL, upload thẳng uniform) ----------
struct Mat4 { float m[16]; };
static Mat4 MatIdentity() {
    Mat4 o{};
    o.m[0] = o.m[5] = o.m[10] = o.m[15] = 1.0f;
    return o;
}
static Mat4 MatMul(const Mat4& a, const Mat4& b) {
    Mat4 o{};
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + r] * b.m[c * 4 + k];
            o.m[c * 4 + r] = s;
        }
    return o;
}
static Mat4 MatPerspective(float fovy, float aspect, float n, float f) {
    Mat4 o{};
    float t = 1.0f / tanf(fovy * 0.5f);
    o.m[0] = t / aspect; o.m[5] = t;
    o.m[10] = (f + n) / (n - f); o.m[11] = -1.0f;
    o.m[14] = (2.0f * f * n) / (n - f);
    return o;
}
static Mat4 MatLookAt(float ex, float ey, float ez, float cx, float cy, float cz) {
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    float fl = sqrtf(fx * fx + fy * fy + fz * fz);
    fx /= fl; fy /= fl; fz /= fl;
    // s = normalize(cross(f, up)), up=(0,1,0); u = cross(s, f)
    float sx = -fz, sy = 0.0f, sz = fx;
    float sl = sqrtf(sx * sx + sy * sy + sz * sz);
    sx /= sl; sy /= sl; sz /= sl;
    float ux = sy * fz - sz * fy, uy = sz * fx - sx * fz, uz = sx * fy - sy * fx;
    Mat4 o = MatIdentity();
    o.m[0] = sx; o.m[4] = sy; o.m[8] = sz;
    o.m[1] = ux; o.m[5] = uy; o.m[9] = uz;
    o.m[2] = -fx; o.m[6] = -fy; o.m[10] = -fz;
    o.m[12] = -(sx * ex + sy * ey + sz * ez);
    o.m[13] = -(ux * ex + uy * ey + uz * ez);
    o.m[14] = fx * ex + fy * ey + fz * ez;
    return o;
}
static Mat4 MatTranslate(float x, float y, float z) {
    Mat4 o = MatIdentity();
    o.m[12] = x; o.m[13] = y; o.m[14] = z;
    return o;
}
static Mat4 MatScale(float x, float y, float z) {
    Mat4 o{};
    o.m[0] = x; o.m[5] = y; o.m[10] = z; o.m[15] = 1.0f;
    return o;
}
static Mat4 MatRotY(float a) {
    Mat4 o = MatIdentity();
    float c = cosf(a), s = sinf(a);
    o.m[0] = c; o.m[2] = -s; o.m[8] = s; o.m[10] = c;
    return o;
}

// ---------- trạng thái app ----------
static bool gInit = false;
static int gFish = 48, gBubbles = 60;
static tglmt::GLuint gProg[7] = {0}; // fish, water, sand, plant, bubble, bg, hud
static tglmt::GLuint gVao[7] = {0}, gVbo[7] = {0}, gEboFish = 0;
static int gDraws = 0; // số draw call / frame (HUD)
static tglmt::GLsizei gFishIndexCount = 0, gWaterIndexCount = 0;
static int gBubbleVboBytes = 0;
struct Fish { float x, y, z, vx, vy, vz, phase, depthPref, scale; };
static std::vector<Fish> gFishes;
struct Bubble { float x, y, z, speed, wob; };
static std::vector<Bubble> gBubs;
struct Plant { float x, z, h; };
static std::vector<Plant> gPlants;
static int64_t gFrames = 0;
static std::string gLastError;
static unsigned gObjMask = 0xFF;
static char gProgStats[256] = {0};
static std::chrono::steady_clock::time_point gT0;
static bool gT0set = false;

static std::string ReadFile(const std::string& p) {
    std::ifstream f(p);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
static tglmt::GLuint MakeProgram(const std::string& dir, const char* vs, const char* fs) {
    using namespace tglmt;
    GLuint v = gl::glCreateShader(0x8B31), f = gl::glCreateShader(0x8B30);
    std::string vss = ReadFile(dir + "/" + vs), fss = ReadFile(dir + "/" + fs);
    const char* vp = vss.c_str();
    const char* fp = fss.c_str();
    gl::glShaderSource(v, 1, &vp, nullptr);
    gl::glShaderSource(f, 1, &fp, nullptr);
    gl::glCompileShader(v);
    gl::glCompileShader(f);
    GLint ok = 0;
    gl::glGetShaderiv(v, 0x8B81, &ok);
    if (!ok) return 0;
    gl::glGetShaderiv(f, 0x8B81, &ok);
    if (!ok) return 0;
    GLuint p = gl::glCreateProgram();
    gl::glAttachShader(p, v);
    gl::glAttachShader(p, f);
    gl::glLinkProgram(p);
    gl::glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) return 0;
    return p;
}
static tglmt::GLint U(tglmt::GLuint p, const char* n) { return tglmt::gl::glGetUniformLocation(p, n); }

// Cá: thân parametric (rings x segs) + vây đuôi + vây lưng. pos.z: mũi +1.2 → đuôi -1.2.
static void BuildFish(std::vector<float>& vtx, std::vector<tglmt::GLuint>& idx) {
    const int R = 14, S = 8;
    const float LEN = 2.4f, MAXR = 0.32f;
    for (int i = 0; i < R; ++i) {
        float t = (float)i / (R - 1);            // 0 mũi → 1 đuôi
        float z = 1.2f - t * LEN;
        float prof = sinf(3.14159f * powf(t, 0.65f)); // phình giữa, nhọn 2 đầu
        float r = MAXR * prof + 0.015f;
        for (int j = 0; j <= S; ++j) {
            float a = (float)j / S * 2.0f * 3.14159f;
            float x = cosf(a) * r, y = sinf(a) * r * 0.82f;
            // normal xuyên tâm (xấp xỉ, đủ cho diffuse)
            float nl = sqrtf(x * x + y * y) + 1e-6f;
            vtx.insert(vtx.end(), {x, y, z, x / nl, y / nl, 0.15f,
                                   0.15f + 0.55f * (1.0f - t), // lưng xanh → bụng bạc
                                   0.45f + 0.35f * (1.0f - t), 0.55f + 0.3f * (1.0f - t),
                                   t * 8.0f}); // a = phase sọc
        }
    }
    for (int i = 0; i < R - 1; ++i)
        for (int j = 0; j < S; ++j) {
            tglmt::GLuint a = i * (S + 1) + j, b = a + S + 1;
            idx.insert(idx.end(), {a, b, a + 1, a + 1, b, b + 1});
        }
    // Vây đuôi: quạt tam giác tại z=-1.2 (wag weight cao nhờ shader smoothstep)
    tglmt::GLuint base = (tglmt::GLuint)vtx.size() / 10;
    float tail[5][10] = {{0, 0, -1.15f, 0, 0, 1, 0.9f, 0.3f, 0.1f, 6.0f},
                         {-0.05f, 0.42f, -1.75f, 0, 0, 1, 0.9f, 0.3f, 0.1f, 6.0f},
                         {0, 0, -1.45f, 0, 0, 1, 0.9f, 0.3f, 0.1f, 6.0f},
                         {0.05f, -0.42f, -1.75f, 0, 0, 1, 0.9f, 0.3f, 0.1f, 6.0f},
                         {0, 0, -1.15f, 0, 0, 1, 0.9f, 0.3f, 0.1f, 6.0f}};
    for (auto& r : tail) vtx.insert(vtx.end(), r, r + 10);
    idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3, base, base + 3, base + 4});
    // Vây lưng: dải quad mỏng trên sống lưng
    base = (tglmt::GLuint)vtx.size() / 10;
    float fin[4][10] = {{0, 0.28f, 0.5f, 0, 1, 0, 0.1f, 0.4f, 0.15f, 4.0f},
                        {0, 0.28f, -0.3f, 0, 1, 0, 0.1f, 0.4f, 0.15f, 4.0f},
                        {0, 0.62f, -0.1f, 0, 1, 0, 0.1f, 0.4f, 0.15f, 4.0f},
                        {0, 0.62f, 0.1f, 0, 1, 0, 0.1f, 0.4f, 0.15f, 4.0f}};
    for (auto& r : fin) vtx.insert(vtx.end(), r, r + 10);
    idx.insert(idx.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

bool Init(const std::string& shaderDir, int fishCount, int bubbleCount) {
    using namespace tglmt;
    if (gInit) return true;
    gFish = fishCount;
    gBubbles = bubbleCount;
    const char* names[7][2] = {{"fish.vert", "fish.frag"}, {"water.vert", "water.frag"},
                               {"sand.vert", "sand.frag"}, {"plant.vert", "plant.frag"},
                               {"bubble.vert", "bubble.frag"}, {"bg.vert", "bg.frag"},
                               {"hud.vert", "hud.frag"}};
    for (int k = 0; k < 7; ++k) {
        gProg[k] = MakeProgram(shaderDir, names[k][0], names[k][1]);
        if (!gProg[k]) {
            char b[128];
            snprintf(b, sizeof(b), "program %d (%s/%s) compile/link FAIL", k, names[k][0],
                     names[k][1]);
            gLastError = b;
            printf("Aquarium: %s\n", b);
            return false;
        }
    }
    // --- fish VAO (pos3/norm3/col4) ---
    {
        std::vector<float> v;
        std::vector<GLuint> ix;
        BuildFish(v, ix);
        gFishIndexCount = (GLsizei)ix.size();
        gl::glGenVertexArrays(1, &gVao[0]);
        gl::glBindVertexArray(gVao[0]);
        gl::glGenBuffers(1, &gVbo[0]);
        gl::glBindBuffer(0x8892, gVbo[0]);
        gl::glBufferData(0x8892, (GLsizeiptr)(v.size() * 4), v.data(), 0x88E4);
        gl::glGenBuffers(1, &gEboFish);
        gl::glBindBuffer(0x8893, gEboFish);
        gl::glBufferData(0x8893, (GLsizeiptr)(ix.size() * 4), ix.data(), 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, 0x1406, 0, 40, (void*)0);
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 3, 0x1406, 0, 40, (void*)12);
        gl::glEnableVertexAttribArray(2);
        gl::glVertexAttribPointer(2, 4, 0x1406, 0, 40, (void*)24);
    }
    // --- water VAO: plane 24x24 tại y=0 (dịch lên mặt nước bằng model) ---
    {
        std::vector<float> v;
        std::vector<GLuint> ix;
        const int N = 24;
        for (int iz = 0; iz <= N; ++iz)
            for (int ix2 = 0; ix2 <= N; ++ix2)
                v.insert(v.end(), {-11.0f + 22.0f * ix2 / N, 0, -8.0f + 16.0f * iz / N});
        for (int iz = 0; iz < N; ++iz)
            for (int ix2 = 0; ix2 < N; ++ix2) {
                GLuint a = iz * (N + 1) + ix2, b = a + N + 1;
                ix.insert(ix.end(), {a, b, a + 1, a + 1, b, b + 1});
            }
        gWaterIndexCount = (GLsizei)ix.size();
        gl::glGenVertexArrays(1, &gVao[1]);
        gl::glBindVertexArray(gVao[1]);
        gl::glGenBuffers(1, &gVbo[1]);
        gl::glBindBuffer(0x8892, gVbo[1]);
        gl::glBufferData(0x8892, (GLsizeiptr)(v.size() * 4), v.data(), 0x88E4);
        tglmt::GLuint ebo;
        gl::glGenBuffers(1, &ebo);
        gl::glBindBuffer(0x8893, ebo);
        gl::glBufferData(0x8893, (GLsizeiptr)(ix.size() * 4), ix.data(), 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    }
    // --- sand VAO: plane 2 tris ---
    {
        float v[] = {-11, -3.2f, -8, 11, -3.2f, -8, 11, -3.2f, 8, -11, -3.2f, -8, 11, -3.2f, 8, -11, -3.2f, 8};
        gl::glGenVertexArrays(1, &gVao[2]);
        gl::glBindVertexArray(gVao[2]);
        gl::glGenBuffers(1, &gVbo[2]);
        gl::glBindBuffer(0x8892, gVbo[2]);
        gl::glBufferData(0x8892, sizeof(v), v, 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    }
    // --- plant VAO: 2 quad chéo cao 4, rộng 0.35 (vUv.x khớp shader) ---
    {
        float v[] = {-0.175f, 0, 0, 0.175f, 0, 0, 0.175f, 4, 0, -0.175f, 0, 0, 0.175f, 4, 0, -0.175f, 4, 0,
                     0, 0, -0.175f, 0, 0, 0.175f, 0, 4, 0.175f, 0, 0, -0.175f, 0, 4, 0.175f, 0, 0, -0.175f};
        gl::glGenVertexArrays(1, &gVao[3]);
        gl::glBindVertexArray(gVao[3]);
        gl::glGenBuffers(1, &gVbo[3]);
        gl::glBindBuffer(0x8892, gVbo[3]);
        gl::glBufferData(0x8892, sizeof(v), v, 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    }
    // --- bubble VAO: dynamic positions ---
    {
        gl::glGenVertexArrays(1, &gVao[4]);
        gl::glBindVertexArray(gVao[4]);
        gl::glGenBuffers(1, &gVbo[4]);
        gl::glBindBuffer(0x8892, gVbo[4]);
        gBubbleVboBytes = gBubbles * 3 * 4;
        std::vector<float> z(gBubbles * 3, 0);
        gl::glBufferData(0x8892, gBubbleVboBytes, z.data(), 0x88E8); // DYNAMIC_DRAW
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    }
    // --- bg VAO: fullscreen triangle ---
    {
        float v[] = {-1, -1, 3, -1, -1, 3};
        gl::glGenVertexArrays(1, &gVao[5]);
        gl::glBindVertexArray(gVao[5]);
        gl::glGenBuffers(1, &gVbo[5]);
        gl::glBindBuffer(0x8892, gVbo[5]);
        gl::glBufferData(0x8892, sizeof(v), v, 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    }
    // --- hud VAO: dynamic quads pos2+col4 ---
    {
        gl::glGenVertexArrays(1, &gVao[6]);
        gl::glBindVertexArray(gVao[6]);
        gl::glGenBuffers(1, &gVbo[6]);
        gl::glBindBuffer(0x8892, gVbo[6]);
        gl::glBufferData(0x8892, 65536 * 6 * 4, nullptr, 0x88E8); // DYNAMIC_DRAW, 4 dòng HUD
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 2, 0x1406, 0, 24, (void*)0);
        gl::glEnableVertexAttribArray(1);
        gl::glVertexAttribPointer(1, 4, 0x1406, 0, 24, (void*)8);
    }
    // --- bầy cá + bọt + rong ---
    srand(12345);
    for (int i = 0; i < gFish; ++i) {
        Fish f;
        f.x = -8 + 16 * (rand() / (float)RAND_MAX);
        f.y = -2 + 4 * (rand() / (float)RAND_MAX);
        f.z = -5 + 10 * (rand() / (float)RAND_MAX);
        float a = (rand() / (float)RAND_MAX) * 6.28f;
        f.vx = cosf(a);
        f.vy = 0;
        f.vz = sinf(a);
        f.phase = (rand() / (float)RAND_MAX) * 6.28f;
        f.depthPref = -2 + 4 * (rand() / (float)RAND_MAX);
        f.scale = 0.7f + 0.7f * (rand() / (float)RAND_MAX);
        gFishes.push_back(f);
    }
    for (int i = 0; i < gBubbles; ++i) {
        Bubble b;
        b.x = -9 + 18 * (rand() / (float)RAND_MAX);
        b.y = -3 + 6 * (rand() / (float)RAND_MAX);
        b.z = -5 + 10 * (rand() / (float)RAND_MAX);
        b.speed = 0.8f + 0.8f * (rand() / (float)RAND_MAX);
        b.wob = (rand() / (float)RAND_MAX) * 6.28f;
        gBubs.push_back(b);
    }
    float px[] = {-7.5f, -4.0f, -1.0f, 2.5f, 5.5f, 8.0f};
    for (float x : px) {
        Plant p;
        p.x = x;
        p.z = -4.5f + (rand() / (float)RAND_MAX) * 3.0f;
        p.h = 0.8f + 0.5f * (rand() / (float)RAND_MAX);
        gPlants.push_back(p);
    }
    gInit = true;
    return true;
}

// Font bitmap 3x5 cho HUD (hàng trên→dưới, bit 1 = pixel). Đủ cho "FPS 59.9 MS 16.9 DRW 62 FISH 48".
static const char* FontGlyph(char c) {
    switch (c) {
        case '0': return "111101101101111";
        case '1': return "010110010010111";
        case '2': return "111001111100111";
        case '3': return "111001111001111";
        case '4': return "101101111001001";
        case '5': return "111100111001111";
        case '6': return "111100111101111";
        case '7': return "111001010010010";
        case '8': return "111101111101111";
        case '9': return "111101111001111";
        case 'F': return "111100110100100";
        case 'P': return "111101111100100";
        case 'S': return "011100010001110";
        case 'M': return "101111111101101";
        case 'D': return "110101101101110";
        case 'R': return "110101110101101";
        case 'A': return "010101111101101";
        case 'W': return "101101101111101";
        case 'I': return "111010010010111";
        case 'H': return "101101111101101";
        case 'G': return "011100101101111";
        case 'L': return "100100100100111";
        case 'T': return "111010010010010";
        case '.': return "000000000000010";
        case ':': return "000010000010000";
        case '/': return "001001010100100";
        default: return "000000000000000"; // space + ký tự lạ
    }
}
// Vẽ text HUD (tọa độ pixel góc-trên-trái) thành quads NDC. Trả số đỉnh.
static int BuildTextQuads(const std::string& text, int px, int py, int scale, int viewW, int viewH,
                          float r, float g, float b, std::vector<float>& out) {
    int n = 0;
    int cx = px;
    for (char c : text) {
        const char* gl = FontGlyph(c);
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 3; ++col) {
                if (gl[row * 3 + col] != '1') continue;
                float x0 = cx + col * scale, y0 = py + row * scale;
                float x1 = x0 + scale, y1 = y0 + scale;
                auto nx = [&](float x) { return (x / viewW) * 2.0f - 1.0f; };
                auto ny = [&](float y) { return 1.0f - (y / viewH) * 2.0f; };
                float q[6][6] = {{nx(x0), ny(y0), r, g, b, 1}, {nx(x1), ny(y0), r, g, b, 1},
                                 {nx(x1), ny(y1), r, g, b, 1}, {nx(x0), ny(y0), r, g, b, 1},
                                 {nx(x1), ny(y1), r, g, b, 1}, {nx(x0), ny(y1), r, g, b, 1}};
                for (auto& v : q)
                    for (float f : v) out.push_back(f);
                n += 6;
            }
        cx += 4 * scale;
    }
    return n;
}
static void DrawHUD(double fps, double ms, int viewW, int viewH) {
    const tglmt::Context::AppleStats* st = nullptr;
    // đọc stats từ Context hiện tại (an toàn khi chưa MakeCurrent: bỏ qua)
    // NOTE: Context::Current() tạo fallback nếu chưa có — ở đây shell đã MakeCurrent.
    st = &tglmt::Context::Current().appleStats;
    using namespace tglmt;
    char l1[32], l2[32];
    snprintf(l1, sizeof(l1), "FPS %.1f", fps);
    snprintf(l2, sizeof(l2), "MS %.1f DRW %d", ms, gDraws);
    std::vector<float> q;
    BuildTextQuads(l1, 12, 12, 3, viewW, viewH, 0.2f, 1.0f, 0.2f, q);
    BuildTextQuads(l2, 12, 12 + 3 * 7, 3, viewW, viewH, 0.2f, 1.0f, 0.2f, q);
    if (st) {
        char l3[48];
        snprintf(l3, sizeof(l3), "GL %llu/%llu P%llu T%llu",
                 (unsigned long long)st->drawsEncoded, (unsigned long long)st->drawsAttempted,
                 (unsigned long long)st->noPipeline, (unsigned long long)st->noTarget);
        BuildTextQuads(l3, 12, 12 + 2 * 3 * 7, 3, viewW, viewH, 1.0f, 1.0f, 0.2f, q);
        // Dòng 4: số draw đã encode theo program (thứ tự fish water sand plant
        // bubble bg hud) — chỉ số, vừa font (không cần chữ thường).
        char l4[64];
        char* w4 = l4;
        size_t left4 = sizeof(l4);
        for (int i = 0; i < 7 && left4 > 2; ++i) {
            auto it = st->progEncoded.find(gProg[i]);
            unsigned long long n = (it == st->progEncoded.end()) ? 0 : it->second;
            int k = snprintf(w4, left4, "%llu ", n);
            if (k < 0) break;
            w4 += k;
            left4 -= (size_t)k;
        }
        BuildTextQuads(l4, 12, 12 + 3 * 3 * 7, 3, viewW, viewH, 1.0f, 0.6f, 0.2f, q);
    }
    gl::glDisable(0x0B71); // HUD không depth
    gl::glDisable(0x0B44); // không cull
    gl::glUseProgram(gProg[6]);
    gl::glBindVertexArray(gVao[6]);
    gl::glBindBuffer(0x8892, gVbo[6]);
    gl::glBufferSubData(0x8892, 0, (GLsizeiptr)(q.size() * 4), q.data());
    gl::glDrawArrays(0x0004, 0, (GLsizei)q.size() / 6);
    gDraws++;
}

static void StepPhysics(double dt, double t) {
    // Boids: separation/alignment/cohesion + tường + wander + độ sâu ưa thích
    const float PR = 2.5f, SEP = 1.5f, ALI = 1.0f, COH = 1.0f;
    const float MAXS = 2.4f, MAXF = 4.0f;
    for (size_t i = 0; i < gFishes.size(); ++i) {
        Fish& f = gFishes[i];
        float sx = 0, sy = 0, sz = 0, ax = 0, ay = 0, az = 0, cx = 0, cy = 0, cz = 0;
        int n = 0;
        for (size_t j = 0; j < gFishes.size(); ++j) {
            if (i == j) continue;
            const Fish& o = gFishes[j];
            float dx = f.x - o.x, dy = f.y - o.y, dz = f.z - o.z;
            float d2 = dx * dx + dy * dy + dz * dz;
            if (d2 < PR * PR && d2 > 1e-6f) {
                float d = sqrtf(d2);
                sx += dx / (d * d);
                sy += dy / (d * d);
                sz += dz / (d * d);
                ax += o.vx;
                ay += o.vy;
                az += o.vz;
                cx += o.x;
                cy += o.y;
                cz += o.z;
                ++n;
            }
        }
        float fx = 0, fy = 0, fz = 0;
        if (n > 0) {
            fx += sx * SEP;
            fy += sy * SEP;
            fz += sz * SEP;
            fx += (ax / n - f.vx) * ALI;
            fy += (ay / n - f.vy) * ALI;
            fz += (az / n - f.vz) * ALI;
            fx += (cx / n - f.x) * 0.3f * COH;
            fy += (cy / n - f.y) * 0.3f * COH;
            fz += (cz / n - f.z) * 0.3f * COH;
        }
        // tường bể [-9,9]x[-3.2,3.6]x[-6,6]
        if (f.x < -7.5f) fx += (-7.5f - f.x) * 3.0f;
        if (f.x > 7.5f) fx -= (f.x - 7.5f) * 3.0f;
        if (f.y < -2.2f) fy += (-2.2f - f.y) * 3.0f;
        if (f.y > 2.6f) fy -= (f.y - 2.6f) * 3.0f;
        if (f.z < -4.5f) fz += (-4.5f - f.z) * 3.0f;
        if (f.z > 4.5f) fz -= (f.z - 4.5f) * 3.0f;
        fy += (f.depthPref - f.y) * 0.4f;
        fx += sinf((float)t * 0.7f + f.phase) * 0.35f;
        fz += cosf((float)t * 0.6f + f.phase * 1.7f) * 0.35f;
        float fl = sqrtf(fx * fx + fy * fy + fz * fz);
        if (fl > MAXF) { fx *= MAXF / fl; fy *= MAXF / fl; fz *= MAXF / fl; }
        f.vx += fx * (float)dt;
        f.vy += fy * (float)dt;
        f.vz += fz * (float)dt;
        float sp = sqrtf(f.vx * f.vx + f.vy * f.vy + f.vz * f.vz) + 1e-6f;
        if (sp > MAXS) { f.vx *= MAXS / sp; f.vy *= MAXS / sp; f.vz *= MAXS / sp; }
        if (sp < 0.6f) { f.vx *= 1.02f; f.vz *= 1.02f; }
        f.x += f.vx * (float)dt;
        f.y += f.vy * (float)dt;
        f.z += f.vz * (float)dt;
        // va chạm cứng thành bể (nảy + kẹp)
        if (f.x < -9) { f.x = -9; f.vx = fabsf(f.vx); }
        if (f.x > 9) { f.x = 9; f.vx = -fabsf(f.vx); }
        if (f.y < -3.2f) { f.y = -3.2f; f.vy = fabsf(f.vy); }
        if (f.y > 3.6f) { f.y = 3.6f; f.vy = -fabsf(f.vy); }
        if (f.z < -6) { f.z = -6; f.vz = fabsf(f.vz); }
        if (f.z > 6) { f.z = 6; f.vz = -fabsf(f.vz); }
    }
    // Bọt khí nổi + lắc, tái sinh ở đáy
    for (auto& b : gBubs) {
        b.y += b.speed * (float)dt;
        b.x += sinf((float)t * 3.0f + b.wob) * 0.15f * (float)dt;
        if (b.y > 3.4f) {
            b.y = -3.0f;
            b.x = -9 + 18 * (rand() / (float)RAND_MAX);
            b.z = -5 + 10 * (rand() / (float)RAND_MAX);
        }
    }
}

bool RenderFrame(double timeSec, double dt, int viewW, int viewH) {
    using namespace tglmt;
    if (!gInit) return false;
    if (!gT0set) { gT0 = std::chrono::steady_clock::now(); gT0set = true; }
    if (dt > 0.05) dt = 0.05;
    if (dt > 0) StepPhysics(dt, timeSec);
    gDraws = 0;
    static double smoothFps = 60.0;
    if (dt > 0) {
        double inst = 1.0 / dt;
        smoothFps = smoothFps * 0.9 + inst * 0.1;
    }

    gl::glViewport(0, 0, viewW, viewH);
    float aspect = viewW / (float)(viewH ? viewH : 1);
    // Camera quay chậm quanh bể
    float camA = (float)timeSec * 0.08f;
    float cex = sinf(camA) * 17.0f, cez = cosf(camA) * 17.0f;
    Mat4 view = MatLookAt(cex, 1.5f, cez, 0, 0.2f, 0);
    Mat4 proj = MatPerspective(0.9f, aspect, 0.5f, 60.0f);
    Mat4 VP = MatMul(proj, view);
    float lightDir[3] = {0.3f, 0.8f, 0.5f};
    float camPos[3] = {cex, 1.5f, cez};

    gl::glClearColor(0.02f, 0.1f, 0.2f, 1.0f);
    gl::glClear(0x00004000 | 0x00000100); // COLOR + DEPTH
    gl::glEnable(0x0B71);                // DEPTH_TEST
    gl::glDepthFunc(0x0201);             // LESS
    gl::glDepthMask(1);
    gl::glEnable(0x0B44); // CULL_FACE (cá kín)
    gl::glCullFace(0x0405);

    // 1. Nền (không depth)
    if (gObjMask & 1) {
    gl::glDisable(0x0B71);
    gl::glUseProgram(gProg[5]);
    gl::glBindVertexArray(gVao[5]);
    {
        Mat4 m = MatIdentity();
        tglmt::GLint l = U(gProg[5], "uMVP");
        (void)l; // bg không dùng MVP (vị trí NDC trực tiếp) — giữ uniform location hợp lệ
        gl::glDrawArrays(0x0004, 0, 3);
        gDraws++;
    }
    gl::glEnable(0x0B71);
    } // mask bg
    // 2. Cát
    if (gObjMask & 2) {
        gl::glUseProgram(gProg[2]);
        gl::glBindVertexArray(gVao[2]);
        tglmt::GLint lm = U(gProg[2], "uMVP"), lt = U(gProg[2], "uTime"), lc = U(gProg[2], "uCamPos");
        gl::glUniformMatrix4fv(lm, 1, 0, VP.m);
        float t = (float)timeSec;
        gl::glUniform1f(lt, t);
        gl::glUniform3f(lc, camPos[0], camPos[1], camPos[2]);
        gl::glDrawArrays(0x0004, 0, 6);
        gDraws++;
    }
    // 3. Rong (6 cây, alpha cutout trong shader, không cần blend)
    if (gObjMask & 4) {
        gl::glUseProgram(gProg[3]);
        gl::glBindVertexArray(gVao[3]);
        tglmt::GLint lm = U(gProg[3], "uMVP"), lt = U(gProg[3], "uTime"), lp = U(gProg[3], "uPhase");
        for (size_t i = 0; i < gPlants.size(); ++i) {
            Mat4 m = MatMul(MatTranslate(gPlants[i].x, -3.2f, gPlants[i].z),
                            MatScale(1.2f, gPlants[i].h, 1.2f));
            Mat4 mvp = MatMul(VP, m);
            gl::glUniformMatrix4fv(lm, 1, 0, mvp.m);
            float t = (float)timeSec;
            gl::glUniform1f(lt, t);
            gl::glUniform1f(lp, (float)i * 1.3f);
            gl::glDrawArrays(0x0004, 0, 12);
            gDraws++;
        }
    }
    // 4. Cá (mỗi con 1 draw, MVP riêng)
    if (gObjMask & 8) {
        gl::glUseProgram(gProg[0]);
        gl::glBindVertexArray(gVao[0]);
        tglmt::GLint lm = U(gProg[0], "uMVP"), lt = U(gProg[0], "uTime"), lw = U(gProg[0], "uWagFreq"),
                      ll = U(gProg[0], "uLightDir");
        gl::glUniform3f(ll, lightDir[0], lightDir[1], lightDir[2]);
        float t = (float)timeSec;
        gl::glUniform1f(lt, t);
        for (auto& f : gFishes) {
            float yaw = atan2f(f.vx, f.vz); // hướng theo vận tốc (+z mũi cá)
            Mat4 m = MatMul(MatTranslate(f.x, f.y, f.z),
                            MatMul(MatRotY(yaw), MatScale(f.scale, f.scale, f.scale)));
            Mat4 mvp = MatMul(VP, m);
            gl::glUniformMatrix4fv(lm, 1, 0, mvp.m);
            float sp = sqrtf(f.vx * f.vx + f.vy * f.vy + f.vz * f.vz);
            gl::glUniform1f(lw, 4.0f + sp * 2.5f);
            gl::glDrawElements(0x0004, gFishIndexCount, 0x1405, (void*)0);
            gDraws++;
        }
    }
    // 5. Mặt nước trong suốt (blend, không ghi depth)
    if (gObjMask & 16) {
        gl::glEnable(0x0BE2); // BLEND
        gl::glBlendFunc(0x0302, 0x0303);
        gl::glDepthMask(0);
        gl::glUseProgram(gProg[1]);
        gl::glBindVertexArray(gVao[1]);
        tglmt::GLint lm = U(gProg[1], "uMVP"), lt = U(gProg[1], "uTime");
        Mat4 m = MatMul(MatTranslate(0, 3.6f, 0), MatIdentity());
        Mat4 mvp = MatMul(VP, m);
        gl::glUniformMatrix4fv(lm, 1, 0, mvp.m);
        float t = (float)timeSec;
        gl::glUniform1f(lt, t);
        gl::glDrawElements(0x0004, gWaterIndexCount, 0x1405, (void*)0);
        gDraws++;
        gl::glDepthMask(1);
    }
    if (gObjMask & 32) {
        // 6. Bọt khí (points, blend giữ nguyên)
        gl::glUseProgram(gProg[4]);
        gl::glBindVertexArray(gVao[4]);
        std::vector<float> bp;
        bp.reserve(gBubs.size() * 3);
        for (auto& b : gBubs) {
            bp.push_back(b.x);
            bp.push_back(b.y);
            bp.push_back(b.z);
        }
        gl::glBindBuffer(0x8892, gVbo[4]);
        gl::glBufferSubData(0x8892, 0, (tglmt::GLsizeiptr)(bp.size() * 4), bp.data());
        tglmt::GLint lm2 = U(gProg[4], "uMVP"), ls = U(gProg[4], "uSize");
        gl::glUniformMatrix4fv(lm2, 1, 0, VP.m);
        gl::glUniform1f(ls, 9.0f); // point size tính bằng pixel (220 trước đây quá to)
        gl::glDrawArrays(0x0000, 0, (tglmt::GLsizei)gBubs.size()); // POINTS
        gDraws++;
        gl::glDisable(0x0BE2);
    }
    // 7. HUD đo benchmark (FPS/ms/draws) — GL thuần, vẽ cuối, không depth
    if (gObjMask & 64) DrawHUD(smoothFps, dt * 1000.0, viewW, viewH);
    if (gFrames == 10) {
        auto& stm = tglmt::Context::Current().appleStats;
        printf("[dbg] total=%llu progs:",
               (unsigned long long)stm.drawsEncoded);
        for (int i = 0; i < 7; ++i) {
            auto it = stm.progEncoded.find(gProg[i]);
            printf(" %u=%llu", gProg[i],
                   (unsigned long long)(it == stm.progEncoded.end() ? 0 : it->second));
        }
        printf("\n");
    }
    // Refresh chuỗi prog stats cho shell log (tên ↔ program id).
    {
        const char* nm[7] = {"fish", "water", "sand", "plant", "bubble", "bg", "hud"};
        const auto& st = tglmt::Context::Current().appleStats;
        char* w = gProgStats;
        size_t left = sizeof(gProgStats);
        for (int i = 0; i < 7 && left > 2; ++i) {
            auto it = st.progEncoded.find(gProg[i]);
            unsigned long long n = (it == st.progEncoded.end()) ? 0 : it->second;
            int k = snprintf(w, left, "%s:%llu ", nm[i], n);
            if (k < 0) break;
            w += k;
            left -= (size_t)k;
        }
    }
    ++gFrames;
    return gl::glGetError() == 0;
}

const char* LastError() { return gLastError.c_str(); }
bool RunSelfTest(int viewW, int viewH, int* r, int* g, int* b) {
    // Tam giác đỏ NDC trực tiếp, không uniform/varying: kiểm tra nguyên stack GL.
    using namespace tglmt;
    static const char* kVS =
        "#version 460 core\n"
        "layout(location = 0) in vec2 pos;\n"
        "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
    static const char* kFS =
        "#version 460 core\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";
    static GLuint prog = 0, vao = 0;
    if (!prog) {
        GLuint vs = gl::glCreateShader(0x8B31), fs = gl::glCreateShader(0x8B30);
        gl::glShaderSource(vs, 1, &kVS, nullptr);
        gl::glShaderSource(fs, 1, &kFS, nullptr);
        gl::glCompileShader(vs);
        gl::glCompileShader(fs);
        prog = gl::glCreateProgram();
        gl::glAttachShader(prog, vs);
        gl::glAttachShader(prog, fs);
        gl::glLinkProgram(prog);
        GLint ok = 0;
        gl::glGetProgramiv(prog, 0x8B82, &ok);
        if (!ok) return false;
        float tri[6] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};
        gl::glGenVertexArrays(1, &vao);
        gl::glBindVertexArray(vao);
        GLuint vbo;
        gl::glGenBuffers(1, &vbo);
        gl::glBindBuffer(0x8892, vbo);
        gl::glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        gl::glEnableVertexAttribArray(0);
        gl::glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    }
    gl::glViewport(0, 0, viewW, viewH);
    gl::glDisable(0x0B71);
    gl::glDisable(0x0BE2);
    gl::glClearColor(0, 0, 0, 1);
    gl::glClear(0x00004000);
    gl::glUseProgram(prog);
    gl::glBindVertexArray(vao);
    gl::glDrawArrays(0x0004, 0, 3);
    if (gl::glGetError() != 0) return false;
    unsigned char px[16];
    memset(px, 0, sizeof(px));
    int cx = viewW / 2, cy = viewH / 2;
    gl::glReadPixels(cx - 1, cy - 1, 2, 2, 0x1908, 0x1401, px);
    // trung bình 4 pixel giữa (chống răng cưa biên): kỳ vọng đỏ trội
    int sr = 0, sg = 0, sb = 0;
    for (int i = 0; i < 4; ++i) { sr += px[4 * i]; sg += px[4 * i + 1]; sb += px[4 * i + 2]; }
    *r = sr / 4; *g = sg / 4; *b = sb / 4;
    return true;
}
void SetObjectMask(unsigned mask) { gObjMask = mask; }
const char* ProgStatsString() { return gProgStats; }
unsigned ProgramId(int index) {
    if (!gInit || index < 0 || index > 6) return 0;
    return gProg[index];
}
bool DumpProgramMSL(unsigned prog, const std::string& dir) {
    std::string err;
    if (!tglmt::DumpProgramMSL(prog, dir, err)) {
        gLastError = "dump MSL prog " + std::to_string(prog) + ": " + err;
        return false;
    }
    return true;
}

double AverageFPS() {
    if (!gT0set || gFrames < 2) return 0;
    double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - gT0).count();
    return el > 0 ? gFrames / el : 0;
}
long RenderedFrames() { return (long)gFrames; }

} // namespace aquarium
