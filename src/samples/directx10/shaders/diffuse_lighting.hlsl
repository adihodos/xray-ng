#pragma pack_matrix row_major

struct VS_IN {
    float3  position : POSITION;
    float3  normal : NORMAL;
    float4  color : COLOR;
};

struct VS_OUT {
    float4 position : SV_Position;
    float3 color : COLOR;
};

cbuffer transforms {
    float4x4 model_view;
    float4x4 normal_view;
    float4x4 model_view_proj;
};

cbuffer light_setings {
    float3 light_pos_view;
    float4 light_ka;
    float4 light_kd;
};

cbuffer material_settings {
    float4 mat_ka;
    float4 mat_kd;
};

VS_OUT main(in VS_IN vs_in) {
    VS_OUT vs_out;

    vs_out.position = mul(model_view_proj, float4(vs_in.position, 1.0f));

    const float3 vertex_eye = mul(model_view, float4(vs_in.position, 1.0f)).xyz;
    const float3 n = normalize(mul(normal_view, float4(vs_in.normal, 0.0f)).xyz);
    const float3 s = normalize(light_pos_view - vertex_eye);
    const float diffuse = max(dot(n, s), 0.0f);

    vs_out.color = (light_ka * mat_ka + diffuse * light_kd * mat_kd).rgb;

    return vs_out;
}
