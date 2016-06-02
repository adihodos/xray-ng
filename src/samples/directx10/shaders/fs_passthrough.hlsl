//#include "vertex_pc.hlsl"

struct vertex_pc_out {
    float4 vertex_pos : SV_Position;
    float3 vertex_color : COLOR;
};

float4 main(in vertex_pc_out ps_in) : SV_Target {
    return float4(ps_in.vertex_color, 1.0f);
}