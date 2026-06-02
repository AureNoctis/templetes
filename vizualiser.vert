#version 460 core

void main(void) {
    int gray = gl_VertexID ^ (gl_VertexID >> 1);

    gl_Position = vec4(
        2.0 * (gray / 2) - 1.0,
        2.0 * (gray % 2) - 1.0,
        0.0,
        1.0
    );
};