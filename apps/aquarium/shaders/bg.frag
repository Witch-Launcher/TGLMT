#version 460 core
layout(location = 0) in float vY;
layout(location = 0) out vec4 fragColor;
void main() {
    vec3 top = vec3(0.1, 0.35, 0.55);
    vec3 bot = vec3(0.01, 0.08, 0.18);
    fragColor = vec4(mix(bot, top, vY * 0.5 + 0.5), 1.0);
}
