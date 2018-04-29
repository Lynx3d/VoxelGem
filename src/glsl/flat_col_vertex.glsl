#version 330

layout(location = 0) in vec3 v_position;
layout(location = 1) in uvec4 v_color;
out vec4 frag_color;

uniform mat4 mvp_mat;
layout (std140) uniform sRGB_LUT
{
	vec4 val[256];
};

void main()
{
	gl_Position = mvp_mat * vec4(v_position, 1);
	frag_color = vec4(val[v_color.r].a, val[v_color.g].a, val[v_color.b].a, float(v_color.a)/255.0);
}
