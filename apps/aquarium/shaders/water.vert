#version 460 core
// Mặt nước: lưới plane, sóng sin theo uTime ở vertex.
layout(location = 0) in vec3 pos;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out float vWave;
layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform float uTime;
void main() {
    vec3 p = pos;
    float w1 = sin(p.x * 1.5 + uTime * 1.2);
    float w2 = sin(p.z * 2.3 - uTime * 0.9);
    p.y += (w1 + w2) * 0.08;
    vWorld = p;
    vWave = (w1 + w2) * 0.5;
    gl_Position = uMVP * vec4(p, 1.0);
}
