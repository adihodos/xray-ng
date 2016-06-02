#include "shaders/cap1/vertex_format.hlsl"

#pragma pack_matrix(row_major)

cbuffer transform_pack : register(b0) {
  float4x4 tf_world_view;
  float4x4 tf_normal_view;
  float4x4 tf_wvp;
};

vertex_pn_out main(in const vertex_pn vs_in) {
  vertex_pn_out vs_out;
  vs_out.clip_pos = mul(tf_wvp, float4(vs_in.position, 1.0f));
  vs_out.position = mul(tf_world_view, float4(vs_in.position, 1.0f)).xyz;
  vs_out.normal   = mul(tf_normal_view, float4(vs_in.normal, 0.0f)).xyz;

  return vs_out;
}

vertex_pnt_out main_pnt(in const vertex_pnt vs_in) {
    vertex_pnt_out vs_out;
    vs_out.pos_clip = mul(tf_wvp, float4(vs_in.position, 1.0f));
    vs_out.pos_eye = mul(tf_normal_view, float4(vs_in.position, 1.0f)).xyz;
    vs_out.normal_eye = mul(tf_normal_view, float4(vs_in.normal, 0.0f)).xyz;
    vs_out.texcoords = vs_in.texcoords;
    
    return vs_out;
}