#ifndef XR_CORE_LIGHT_TYPES_GLSL_INCLUDED
#define XR_CORE_LIGHT_TYPES_GLSL_INCLUDED

struct DirectionalLight
{
    vec3 L;
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
};

struct PointLight
{
    vec3 P;
    float range;
    vec3 attenuation;
    vec4 Ka;
    vec4 Kd;
    vec4 Ks;
};

struct SpotLight
{
    vec3 position;
    float range;
    vec3 direction;
    float spot;
    vec3 attenuation;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

#endif /* !defined XR_CORE_LIGHT_TYPES_GLSL_INCLUDED */
