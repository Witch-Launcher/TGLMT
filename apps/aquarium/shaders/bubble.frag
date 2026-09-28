#version 460 core
layout(location = 0) in float vA;
layout(location = 0) out vec4 fragColor;
void main() {
    vec2 d = gl_PointCoord - vec2(0.5);
    if (dot(d, d) > 0.25) discard;
    fragColor = vec4(0.7, 0.9, 1.0, vA);
}
