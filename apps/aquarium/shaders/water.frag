#version 460 core
layout(location = 0) in vec3 vWorld;
layout(location = 1) in float vWave;
layout(location = 0) out vec4 fragColor;
void main() {
    vec3 deep = vec3(0.05, 0.25, 0.45);
    vec3 shal = vec3(0.2, 0.55, 0.7);
    vec3 col = mix(deep, shal, 0.5 + 0.5 * vWave);
    float spec = pow(max(0.0, vWave), 8.0);
    fragColor = vec4(col + spec * 0.6, 0.55);
}
