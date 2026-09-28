#version 460 core
// Cá: thân parametric (pos/normal/color), đuôi vẫy theo uTime.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec4 vColor;
layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform float uTime;
layout(location = 2) uniform float uWagFreq;
void main() {
    vec3 p = pos;
    // smoothstep yêu cầu edge0 < edge1 (GLSL spec: ngược lại là undefined —
    // Apple compiler cho ra rác). Đuôi ở z âm → weight cao ở đuôi:
    float w = 1.0 - smoothstep(-1.2, 0.2, p.z);
    p.x += sin(uTime * uWagFreq + p.z * 3.0) * 0.25 * w;
    vNormal = normal;
    vColor = color;
    gl_Position = uMVP * vec4(p, 1.0);
}
