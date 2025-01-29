#ifndef XR_CORE_LIGHT_TYPES_GLSL_INCLUDED
#define XR_CORE_LIGHT_TYPES_GLSL_INCLUDED

struct DirectionalLight
{
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight
{
    vec3 position;
    float range;
    vec3 attenuation;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
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
