#version 410

uniform sampler2D texture_to_load;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;


void main()
{
	frag_color = texture(texture_to_load, fs_in.texcoord);
	frag_color.w = 1.0;
}
