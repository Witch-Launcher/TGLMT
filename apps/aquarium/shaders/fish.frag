#version 460 core
layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec4 vColor;
layout(location = 0) out vec4 fragColor;
layout(location = 0) uniform vec3 uLightDir;
void main() {
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3 base = vColor.rgb * (0.35 + 0.65 * diff);
    float stripe = 0.85 + 0.15 * sin(vColor.a * 40.0);
    fragColor = vec4(base * stripe, 1.0);
}
