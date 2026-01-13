	const uint img_output_id = g_PushConsts.packed0 & 0xFF;
	const uint resolution = (g_PushConsts.packed0 >> 8) & 0xFFFF;
	const uint face_index = (g_PushConsts.packed0 >> 24);
	const uint seed = g_PushConsts.packed1 & 0xFFFF;

	const vec2 uv = vec2(float(gl_GlobalInvocationID.x) / float(resolution), 1.0 - float(gl_GlobalInvocationID.y) / float(resolution));
	const vec3 sphericalCoord = getSphericalCoord(face_index, uv.x * float(resolution), uv.y * float(resolution), float(resolution));
