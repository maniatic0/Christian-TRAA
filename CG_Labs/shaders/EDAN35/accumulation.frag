#version 410

uniform sampler2D deferred_texture;
uniform float samples_inverse;

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 accumulation_texture;

void main()
{
	accumulation_texture = texture(deferred_texture, fs_in.texcoord) * samples_inverse;
	accumulation_texture.w = 1.0;
}
