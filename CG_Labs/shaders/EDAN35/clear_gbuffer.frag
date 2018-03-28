#version 410

uniform uint clear_id;
uniform vec4 clear_color;

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 geometry_diffuse;
layout (location = 1) out vec4 geometry_specular;
layout (location = 2) out vec4 geometry_normal;
layout (location = 3) out vec2 velocity_buffer;
layout (location = 4) out uint model_index_buffer;


void main()
{
	

	// Model Index
	model_index_buffer = clear_id;

	// Diffuse color
	geometry_diffuse = clear_color;

	// Specular color
	geometry_specular = clear_color;

	// Worldspace normal
	geometry_normal = clear_color;

    velocity_buffer = clear_color.xy;
}
