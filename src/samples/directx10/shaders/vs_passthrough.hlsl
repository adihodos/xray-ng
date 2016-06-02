//#include "vertex_pc.hlsl"
#pragma pack_matrix row_major

struct vertex_pc_in {
    float3 vertex_pos : POSITION;
    float3 vertex_color : COLOR;
};

struct vertex_pc_out {
    float4 vertex_pos : SV_Position;
    float3 vertex_color : COLOR;
};

cbuffer transforms : register(cb0) {
    float2x2 rotation;
};

vertex_pc_out main(in vertex_pc_in vs_in) {
    const float2 tf_point = vs_in.vertex_pos.xy;
        // mul(rotation, vs_in.vertex_pos.xy);
    vertex_pc_out vs_out;
    vs_out.vertex_pos = float4(mul(rotation, tf_point), 0.0f, 1.0f);
    vs_out.vertex_color = //float4(mul(rotation, tf_point), 0.0f, 1.0f);
        vs_in.vertex_color;

    return vs_out;
}