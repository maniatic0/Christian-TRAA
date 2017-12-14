#version 410

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normals_texture;
uniform sampler2D opacity_texture;
uniform bool has_opacity_texture;
uniform mat4 normal_model_to_world;

in VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec4 current_pos;
	vec4 old_pos;
} fs_in;

layout (location = 0) out vec4 geometry_diffuse;
layout (location = 1) out vec4 geometry_specular;
layout (location = 2) out vec4 geometry_normal;
layout (location = 3) out vec2 velocity_buffer;


void main()
{
	if (has_opacity_texture && texture(opacity_texture, fs_in.texcoord).r < 1.0)
		discard;

	// Diffuse color
	geometry_diffuse = texture(diffuse_texture, fs_in.texcoord);

	// Specular color
	geometry_specular = texture(specular_texture, fs_in.texcoord);

	// Worldspace normal
	geometry_normal.xyz = (2.0 * texture(normals_texture, fs_in.texcoord).xyz) - vec3(1.0, 1.0, 1.0);

	vec3 N = normalize(fs_in.normal);
	vec3 B = normalize(fs_in.binormal);
	vec3 T = normalize(fs_in.tangent);
	mat4 TBN = mat4(
		vec4(T, 0.0), 
		vec4(B, 0.0),
		vec4(N, 0.0),
		vec4(0.0,0.0,0.0, 1.0)
		);

	geometry_normal.xyz = (
		normal_model_to_world 
		* TBN
		* vec4(geometry_normal.xyz, 0.0)
		).xyz;
	geometry_normal.xyz = (normalize(geometry_normal.xyz) * 0.5) + vec3(0.5, 0.5, 0.5);

    vec2 a = (fs_in.current_pos.xy / fs_in.current_pos.w);
    vec2 b = (fs_in.old_pos.xy / fs_in.old_pos.w);
    velocity_buffer = (a - b);
}
