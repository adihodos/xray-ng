
gl_Position = fgd.world_view_proj * inst.model * vec4(pos, 1.0);
vs_out.P = (inst.model_view * vec4(pos, 1.0)).xyz;
vs_out.N = normalize(mat3(inst.normals_view) * normal);
vs_out.V = normalize(-vs_out.P);
vs_out.uv = uv;
vs_out.fii = uvec3(frame_idx, inst_buffer_idx, instance_index);

