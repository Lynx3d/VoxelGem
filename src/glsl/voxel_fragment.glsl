#version 330
in vec4 frag_color;
in vec3 frag_normal;
in vec3 frag_pos_view;
flat in uint frag_mat_index;
in float frag_occlusion;
out vec4 final_color;

uniform mat4 view_mat;

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
	// we calculate in vew space; TODO: we should transform lights outside the fragment shader...
	vec3 ldir = mat3(view_mat) * light_dir;
	// hardcoded 0.2 ambient for now...
	final_color = frag_color * (0.2 * frag_occlusion + max(0, dot(ldir, normalize(frag_normal)))) * light_col;
	final_color += frag_color * mat_prop[frag_mat_index].emit;
	// specular
	vec3 viewdir = normalize(-frag_pos_view);
	vec3 reflected = reflect(-ldir, frag_normal);
	float spec = pow(max(dot(viewdir, reflected), 0.0), mat_prop[frag_mat_index].spec_sharpness);
	final_color += spec * mat_prop[frag_mat_index].spec_amount * light_col;
}
