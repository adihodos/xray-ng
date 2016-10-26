#version 450 core

layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexTexCoord;

layout(location=0) out vec3 OutputTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    OutputTexCoord = VertexTexCoord;
    gl_Position = vec4(VertexPosition, 1.0f);
} 