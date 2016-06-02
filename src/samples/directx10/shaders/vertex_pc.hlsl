#ifndef vertex_pc_hlsl__
#define vertex_pc_hlsl__

struct vertex_pc_in {
    float3 vertex_pos : POSITION;
    float3 vertex_color : COLOR;
};

struct vertex_pc_out {
    float4 vertex_pos : SV_Position;
    float3 vertex_color : COLOR;
};

#endif // vertex_pc_hlsl__