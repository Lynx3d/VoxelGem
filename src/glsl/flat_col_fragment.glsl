#version 330

//VOXELGEM_DEFINES

in vec4 frag_color;
out vec4 final_color;

void main()
{
#ifndef QT_5_10
	final_color = vec4(pow(frag_color.rgb, vec3(1.0/2.2)), frag_color.a);
#else
	final_color = frag_color;
#endif
}
