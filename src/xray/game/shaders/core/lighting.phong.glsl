
vec4 light_phong(
          in vec3 P
        , in vec3 V
        , in vec3 N
        , in vec4 Ka
        , in vec4 Kd
        , in vec4 Ks
        , in uvec2 dir_lights
        , in uvec2 point_lights
        ) {
    vec4 color = vec4(0);

    //
    // directional
    for (uint i = 0; i < dir_lights.x; ++i) {
        const DirectionalLight dl = g_DirectionalLights[dir_lights.y].lights[i];
        color += dl.Ka * Ka + max(0, dot(-dl.L, N)) * Kd * dl.Kd;
        color += pow(max(0.0, dot(reflect(dl.L, N), V)), 128.0) * dl.Ks * Ks;
    }

    //
    // point
    for (uint i = 0; i < point_lights.x; ++i) {
        const PointLight pl = g_PointLights[point_lights.y].lights[i];
        const vec3 L = normalize(pl.P - P);
        color += pl.Ka * Ka + max(0, dot(L, N)) * Kd * pl.Kd;
        color += pow(max(0.0, dot(reflect(-L, N), V)), 128.0) * pl.Ks * Ks;
    }

    return color;
}

