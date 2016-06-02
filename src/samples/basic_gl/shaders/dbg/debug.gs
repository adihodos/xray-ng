#version 450 core

layout (row_major) uniform;
layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

in VS_OUT_GS_IN {
     vec3 pos;
     vec3 normal;
     vec2 texc;
} gs_in[];

out GS_OUT_PS_IN {
    layout (location = 0) vec4 color;
} gs_out;

layout (binding = 0) uniform transform_pack {
    mat4 model_view_projection;
};

uniform float normal_length;
uniform vec4 start_point_color;
uniform vec4 end_point_color;

void main() {
    const vec3 triangle_centroid =
        (gs_in[0].pos + gs_in[1].pos + gs_in[2].pos) * 0.33333f;

    {
        gl_Position = model_view_projection * vec4(triangle_centroid, 1.0f);
        gs_out.color = start_point_color;
        EmitVertex();
    }

    {
        const vec3 avg_normal = normalize(
            gs_in[0].normal + gs_in[1].normal + gs_in[2].normal);

        const vec3 end_pt = triangle_centroid + avg_normal * normal_length;

        gl_Position = model_view_projection * vec4(end_pt, 1.0f);
        gs_out.color = end_point_color;

        EmitVertex();
    }
    
    EndPrimitive();
}
