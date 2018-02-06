#version 330
in vec4 frag_color;
in vec3 frag_normal;
flat in uint frag_mat_index;
out vec4 final_color;

struct Material
{
	float spec_amount;
	float spec_sharpness;
	float spec_tinting;
	float emit;
};

layout (std140) uniform materials
{
	Material mat_prop[10];
};

// TODO: light is fixed for test rather than input
//uniform vec3 light_dir;
//uniform vec4 light_col;
const vec3 light_dir = vec3(1.0/sqrt(14) , 3.0/sqrt(14) , -2.0/sqrt(14) );
const vec4 light_col = vec4(0.8, 0.8, 0.8, 1.0);

void main()
{
	// hardcoded 0.2 diffuse for now...
	final_color = frag_color * (0.2 + max(0, dot(light_dir, normalize(frag_normal)))) * light_col;
	final_color += frag_color * mat_prop[frag_mat_index].emit;
}
