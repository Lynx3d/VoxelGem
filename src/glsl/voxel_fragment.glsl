#version 330
in vec4 frag_color;
in vec3 frag_normal;
in vec3 frag_tangent;
in vec3 frag_pos_view;
in vec3 frag_uv;
flat in uint frag_mat_index;
in float frag_occlusion;
out vec4 final_color;

uniform mat4 view_mat;
uniform sampler2DArray normal_tex;

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
const vec4 light_col = vec4(0.7, 0.7, 0.7, 1.0);

void main()
{
	// apply normal mapping:
	vec4 tex_col = texture(normal_tex, frag_uv);
	vec3 t_norm = normalize(tex_col.rgb - vec3(127.0/255.0));
	vec3 bi_norm = cross(frag_normal, frag_tangent);
	vec3 normal = t_norm.x * frag_tangent + t_norm.y * bi_norm + t_norm.z * frag_normal;

	Material m_prop = mat_prop[frag_mat_index];
	// we calculate in vew space; TODO: we should transform lights outside the fragment shader...
	vec3 ldir = mat3(view_mat) * light_dir;
	// hardcoded 0.3 "grey" ambient for now...
	float ambient = 0.3 * frag_occlusion;
	vec4 diffuse_col = frag_color * max(0, dot(ldir, normalize(normal))) * light_col;
	final_color = frag_color * ambient + mix(diffuse_col, frag_color * (tex_col.a + 0.1), m_prop.emit);
	// specular
	vec3 viewdir = normalize(-frag_pos_view);
	/*vec3 reflected = reflect(-ldir, normal);
	float spec = pow(max(dot(viewdir, reflected), 0.0), m_prop.spec_sharpness);*/
	//Blinn Phong:
	vec3 half_dir = normalize(viewdir + ldir);
	float spec_angle = max(dot(half_dir, normal), 0.0);
	float spec = pow(spec_angle, 3.0 * m_prop.spec_sharpness);
	final_color += spec * m_prop.spec_amount * light_col * mix(vec4(1.0), frag_color, m_prop.spec_tinting);
	// Trove renders specular highlights increasingly opaque rather than as additive effect
	final_color.a = frag_color.a + spec;
}
