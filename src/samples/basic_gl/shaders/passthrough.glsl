#version 430 core

layout(location=0) in vec2 VertexPos;
layout(location=1) in vec3 VertexColor;

out vec4 VertexOutputColor;

void main() {
    gl_Position = vec4(VertexPos, 0.0f, 1.0f);
    VertexOutputColor = vec4(VertexColor, 1.0f);
}