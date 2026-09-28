#version 460 core
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;
void main() {
    if (vUv.x < 0.02 || vUv.x > 0.98) discard;
    vec3 col = mix(vec3(0.05, 0.3, 0.05), vec3(0.2, 0.7, 0.2), vUv.y);
    fragColor = vec4(col, 1.0);
}
