#version 460 core
// Rong: 2 quad chéo, đung đưa theo uTime (sway tăng dần theo y).
layout(location = 0) in vec3 pos;
layout(location = 0) out vec2 vUv;
layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform float uTime;
layout(location = 2) uniform float uPhase;
void main() {
    vec3 p = pos;
    float sway = p.y * 0.3;
    p.x += sin(uTime * 1.5 + uPhase) * sway;
    p.z += cos(uTime * 1.1 + uPhase) * sway * 0.5;
    vUv = vec2(pos.x * 2.857 + 0.5, pos.y * 0.25);
    gl_Position = uMVP * vec4(p, 1.0);
}
