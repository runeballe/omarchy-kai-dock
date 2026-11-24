#version 440
// Ripples an icon: a travelling sine wave bends its rows sideways and its
// columns up and down, swelling in and dying out over one run (phase 0..1).
layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;
layout(location = 0) out vec2 coord;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float phase;
    float amplitude;
    float w;
    float h;
};
void main() {
    coord = qt_MultiTexCoord0;
    vec4 pos = qt_Vertex;
    float envelope = sin(3.14159265 * phase);
    float travel = phase * 3.0;
    pos.x += amplitude * w * envelope * sin(6.2831853 * (coord.y * 1.2 - travel));
    pos.y += amplitude * h * 0.6 * envelope * sin(6.2831853 * (coord.x * 1.2 - travel));
    gl_Position = qt_Matrix * pos;
}
