#version 460 core

#include "core/bindless.core.glsl"
#include "core/std.interface.glsl"

const float M_SQRT_2 = 1.4142135623730951;

#if defined(__VERT_SHADER__)

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

#endif

INTERFACE_BLOCK
VS_OUT_FS_IN {
layout(location = 0) vec2 rotation;
layout(location = 1) vec4 fg_color;
layout(location = 2) vec4 bg_color;
layout(location = 3) float size;
layout(location = 4) float v_size;
layout(location = 5) float linewidth;
layout(location = 6) float antialias;
layout(location = 7) flat uint shape_kind;
} BLOCKVAR;

#if defined(__VERT_SHADER__)
void main() {
    #include "core/unpack.push.const.glsl"
    const ShapeSetup shape = g_ShapesBufferGlobal[inst_buffer_idx].data[gl_InstanceIndex];

    gl_Position = vec4((fgd.ortho_proj * vec4(shape.position, 1.0)).xy, 0.0, 1.0);

    vs_out.rotation = vec2(shape.cos_theta, shape.sin_theta);
    vs_out.size = shape.size;
    vs_out.v_size = M_SQRT_2 * shape.size + 2.0 * (shape.line_width + 1.5 * shape.antialias);
    gl_PointSize = vs_out.v_size;

    vs_out.fg_color = unpackUnorm4x8(shape.fg_color);
    vs_out.bg_color = unpackUnorm4x8(shape.bg_color);
    vs_out.linewidth = shape.line_width;
    vs_out.antialias = shape.antialias;
    vs_out.shape_kind = shape.kind;
}
#else

#define ShapeKind_Disc 0
#define ShapeKind_Square 1
#define ShapeKind_Triangle 2
#define ShapeKind_Diamond 3
#define ShapeKind_Chevron 4
#define ShapeKind_Ring 5
#define ShapeKind_Tag 6
#define ShapeKind_Cross 7
#define ShapeKind_Asterisk 8
#define ShapeKind_Infinity 9
#define ShapeKind_BlockArrow 10

layout(location = 0) out vec4 FinalFragColor;

vec4 outline(
    float dist, // Signed distance to line
    float linewidth, // Stroke line width
    float antialias, // Stroke antialiased area
    vec4 stroke, // Stroke color
    vec4 fill) // Fill color
{
    float t = linewidth / 2.0 - antialias;
    float signed_distance = dist;
    float border_distance = abs(signed_distance) - t;
    float alpha = border_distance / antialias;
    alpha = exp(-alpha * alpha);

    if (border_distance < 0.0)
        return stroke;
    else if (signed_distance < 0.0)
        return mix(fill, stroke, sqrt(alpha));
    else
        return vec4(stroke.rgb, stroke.a * alpha);
}

float marker_disc(const vec2 P, const float size) {
    return length(P) - size * 0.5;
}

float marker_square(const vec2 P, const float size) {
    return max(abs(P.x), abs(P.y)) - size / (2.0 * M_SQRT_2);
}

float marker_triangle(const vec2 P, const float size) {
    float x = M_SQRT_2 / 2.0 * (P.x - P.y);
    float y = M_SQRT_2 / 2.0 * (P.x + P.y);
    float r1 = max(abs(x), abs(y)) - size / (2 * M_SQRT_2);
    float r2 = P.y;
    return max(r1, r2);
}

float marker_diamond(const vec2 P, const float size) {
    float x = M_SQRT_2 / 2.0 * (P.x - P.y);
    float y = M_SQRT_2 / 2.0 * (P.x + P.y);
    return max(abs(x), abs(y)) - size / (2.0 * M_SQRT_2);
}

float marker_chevron(const vec2 P, const float size) {
    float x = 1.0 / M_SQRT_2 * (P.x - P.y);
    float y = 1.0 / M_SQRT_2 * (P.x + P.y);
    float r1 = max(abs(x), abs(y)) - size / 3.0;
    float r2 = max(abs(x - size / 3.0), abs(y - size / 3.0)) - size / 3.0;
    return max(r1, -r2);
}

float marker_ring(const vec2 P, const float size) {
    float r1 = length(P) - size / 2.0;
    float r2 = length(P) - size / 4.0;
    return max(r1, -r2);
}

float marker_tag(const vec2 P, const float size) {
    float r1 = max(abs(P.x) - size / 2.0, abs(P.y) - size / 6.0);
    float r2 = abs(P.x - size / 1.5) + abs(P.y) - size;
    return max(r1, 0.75 * r2);
}

float marker_cross(const vec2 P, const float size) {
    float x = M_SQRT_2 / 2.0 * (P.x - P.y);
    float y = M_SQRT_2 / 2.0 * (P.x + P.y);
    float r1 = max(abs(x - size / 3.0), abs(x + size / 3.0));
    float r2 = max(abs(y - size / 3.0), abs(y + size / 3.0));
    float r3 = max(abs(x), abs(y));
    return max(min(r1, r2), r3) - size / 2.0;
}

float marker_asterisk(const vec2 P, const float size) {
    float x = M_SQRT_2 / 2.0 * (P.x - P.y);
    float y = M_SQRT_2 / 2.0 * (P.x + P.y);
    float r1 = max(abs(x) - size / 2.0, abs(y) - size / 10.0);
    float r2 = max(abs(y) - size / 2.0, abs(x) - size / 10.0);
    float r3 = max(abs(P.x) - size / 2.0, abs(P.y) - size / 10.0);
    float r4 = max(abs(P.y) - size / 2.0, abs(P.x) - size / 10.0);
    return min(min(r1, r2), min(r3, r4));
}

float marker_infinity(const vec2 P, const float size) {
    const vec2 c1 = vec2(+0.2125, 0.00);
    const vec2 c2 = vec2(-0.2125, 0.00);
    float r1 = length(P - c1 * size) - size / 3.5;
    float r2 = length(P - c1 * size) - size / 7.5;
    float r3 = length(P - c2 * size) - size / 3.5;
    float r4 = length(P - c2 * size) - size / 7.5;
    return min(max(r1, -r2), max(r3, -r4));
}

float marker_block_arrow(const vec2 P, const float size) {
    float x = P.x;
    float y = P.y;
    float r1 = abs(x) + abs(y) - size / 2;
    float r2 = max(abs(x + size / 2), abs(y)) - size / 2;
    float r3 = max(abs(x - size / 6) - size / 4, abs(y) - size / 4);
    return min(r3, max(0.75 * r1, r2));
}

void main() {
    vec2 P = gl_PointCoord.xy - vec2(0.5);
    const vec2 R = fs_in.rotation;
    P = vec2(R.x * P.x - R.y * P.y, R.y * P.x + R.x * P.y);
    float dist = 0;

    switch (fs_in.shape_kind) {
        default:
        case ShapeKind_Disc:
        dist = marker_disc(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Square:
        dist = marker_square(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Triangle:
        dist = marker_triangle(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Diamond:
        dist = marker_diamond(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Chevron:
        dist = marker_chevron(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Ring:
        dist = marker_ring(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Tag:
        dist = marker_tag(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Cross:
        dist = marker_cross(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Asterisk:
        dist = marker_asterisk(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_Infinity:
        dist = marker_infinity(P * fs_in.v_size, fs_in.size);
        break;

        case ShapeKind_BlockArrow:
        dist = marker_block_arrow(P * fs_in.v_size, fs_in.size);
        break;
    }

    FinalFragColor = outline(dist, fs_in.linewidth, fs_in.antialias, fs_in.fg_color, fs_in.bg_color);
}

#endif
