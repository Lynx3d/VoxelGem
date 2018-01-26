#version 330

layout(location = 0) in vec3 v_position;
layout(location = 1) in uvec4 v_color;
layout(location = 2) in vec3 v_normal;
out vec4 frag_color;
out vec3 frag_normal;

uniform mat4 mvp_mat;
layout (std140) uniform sRGB_LUT
{
	float val[256];
};

void main()
{
	gl_Position = mvp_mat * vec4(v_position, 1);
	frag_color = vec4(val[v_color.r], val[v_color.g], val[v_color.b], float(v_color.a)/255.0);
	frag_normal = v_normal;
}
