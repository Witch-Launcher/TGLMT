#version 460 core
// Output DUY NHẤT ở location=1 — phải ghi vào attachment 1, không phải 0.
layout(location = 1) out vec4 oColor;
void main() { oColor = vec4(0.0, 1.0, 0.0, 1.0); }
