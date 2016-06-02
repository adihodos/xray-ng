#ifndef __vertex_format_hlsl__
#define __vertex_format_hlsl__

struct vertex_pn {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vertex_pn_out {
    float4 clip_pos : SV_Position;
    float3 position : POSITION_EYE;
    float3 normal : NORMAL_EYE;
};

struct vertex_pnt {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoords : TEXCOORDS;
};

struct vertex_pnt_out {
    float4 pos_clip : SV_Position;
    float3 pos_eye : POSITION_EYE;
    float3 normal_eye : NORMAL_EYE;
    float2 texcoords : TEXCOORDS;
};

#endif /* !defined __vertex_format_hlsl__ */