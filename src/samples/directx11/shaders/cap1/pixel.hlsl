#include "shaders/cap1/vertex_format.hlsl"

Texture2D diffuse_mtl;
SamplerState smpl;

float4 main(in const vertex_pn_out ps_in) : SV_Target {
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}

float4 main_pnt(in const vertex_pnt_out ps_in) : SV_Target {
    return diffuse_mtl.Sample(smpl, ps_in.texcoords);
}

