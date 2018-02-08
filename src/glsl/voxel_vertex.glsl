#version 330

layout(location = 0) in vec3 v_position;
layout(location = 1) in uvec4 v_color;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in uint v_mat_index;
layout(location = 4) in float v_occlusion;
out vec4 frag_color;
out vec3 frag_normal;
out vec3 frag_pos_view;
flat out uint frag_mat_index;
out float frag_occlusion;

uniform mat4 mvp_mat;
uniform mat4 view_mat;

layout (std140) uniform sRGB_LUT
{
	float val[256];
};

void main()
{
	gl_Position = mvp_mat * vec4(v_position, 1);
	frag_color = vec4(val[v_color.r], val[v_color.g], val[v_color.b], float(v_color.a)/255.0);
	frag_pos_view = vec3(view_mat * vec4(v_position, 1));
	frag_normal = mat3(view_mat) * v_normal;
	// occlusion goes from 0 (no occlusion) to 3 (occluded quadrants), 4 quadrants considered
	frag_occlusion = 0.25 * (4 - v_occlusion);
	// just forward
	frag_mat_index = v_mat_index;
}
