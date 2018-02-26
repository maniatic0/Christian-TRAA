#version 410

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D light_d_texture;
uniform sampler2D light_s_texture;

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 deferred_buffer;

void main()
{
	vec3 diffuse  = texture(diffuse_texture,  fs_in.texcoord).rgb;
	vec3 specular = texture(specular_texture, fs_in.texcoord).rgb;

	vec3 light_d  = texture(light_d_texture,  fs_in.texcoord).rgb;
	vec3 light_s  = texture(light_s_texture,  fs_in.texcoord).rgb;
	const vec3 ambient = vec3(0.15);

	//deferred_buffer =  vec4((ambient + light_d) * diffuse + light_s * specular, 1.0);
	deferred_buffer =  vec4((ambient + light_d) * diffuse, 1.0);
}
