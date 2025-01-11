#ifndef XR_VK_BINDLESS_CORE_GLSL_INCLUDED
#define XR_VK_BINDLESS_CORE_GLSL_INCLUDED

#extension GL_EXT_nonuniform_qualifier : require

struct UIData {
    vec2 scale;
    vec2 translate;
    uint textureid;
};

struct FrameGlobalData_t {
    mat4 world_view_proj;
    mat4 projection;
    mat4 inv_projection;
    mat4 view;
    mat4 ortho_proj;
    vec3 eye_pos;
    uint frame_id;
    uint global_color_texture;
    UIData ui;
};

layout (std140, set = 0, binding  = 0, row_major) uniform FrameGlobal {
    FrameGlobalData_t data[];
} g_FrameGlobal[];

struct InstanceRenderInfo_t {
    mat4 model;
    uint vtx_buff;
    uint idx_buff;
    uint mtl_coll_offset;
    uint mtl_id;
};

layout (set = 1, binding = 0, row_major) readonly buffer InstancesGlobal {
    InstanceRenderInfo_t data[];
} g_InstanceGlobal[];

layout (set = 1, binding = 0) readonly buffer GlobalUIRenderData {
    UIData data[];
} g_UIRenderDataGlobal[];

struct VertexPTC {
    vec2 pos;
    vec2 uv;
    vec4 color;
};

struct VertexPBR {
    vec3 pos;
    vec3 normal;
    vec4 color;
    vec4 tangent;
    vec2 uv;
    uint pbr_buf_id;
};

struct MaterialADSColored {
    uint texel_offset;
};

struct MaterialADSTextured {
    uint ambient;
    uint diffuse;
    uint specular;
};

struct PBRMaterial {
    vec4 base_color_factor;
    uint base_color;
    uint metallic;
    uint normal;
    float metallic_factor;
    float roughness_factor;
};

layout (set = 1, binding = 0) readonly buffer VerticesGlobal {
    VertexPTC data[];
} g_VertexBufferGlobal[];

layout (set = 1, binding = 0) readonly buffer VerticesPBRGlobal {
    VertexPBR data[];
} g_VertexBufferPBRGlobal[];

layout (set = 1, binding = 0) readonly buffer IndicesGlobal {
    uint data[];
} g_IndexBufferGlobal[];

layout (set = 1, binding = 0) readonly buffer PBRMaterialGlobal {
    PBRMaterial data[];
} g_PBRMaterialGlobal;

layout (set = 1, binding = 0) readonly buffer MaterialADSColoredGlobal {
    MaterialADSColored data[];
} g_MaterialADSColoredGlobal;

layout (set = 1, binding = 0) readonly buffer MaterialADSTexturedGlobal {
    MaterialADSTextured data[];
} g_MaterialADSTexturedGlobal;

layout (set = 2, binding = 0) uniform sampler1D g_Textures1DGlobal[];
layout (set = 2, binding = 0) uniform sampler2D g_Textures2DGlobal[];
layout (set = 2, binding = 0) uniform sampler2DArray g_Textures2DArrayGlobal[];
layout (set = 2, binding = 0) uniform samplerCube g_TexturesCubeGlobal[];

layout (push_constant) uniform PushConstantsGlobal {
    uint data;
} g_GlobalPushConst;

#endif /* !defined XR_VK_BINDLESS_CORE_GLSL_INCLUDED */
