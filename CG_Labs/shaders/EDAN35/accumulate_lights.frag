#version 410

#define M_PI 3.1415926535897932384626433832795
#define DEPTH_EPSILON 0.00001

uniform sampler2D depth_texture;
uniform sampler2D normal_texture;
uniform sampler2DShadow shadow_texture;

uniform vec2 inv_res;

uniform mat4 view_projection_inverse;
uniform vec3 camera_position;
uniform mat4 shadow_view_projection;

uniform vec3 light_color;
uniform vec3 light_position;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float light_angle_falloff;

uniform vec2 shadowmap_texel_size;

layout (location = 0) out vec4 light_diffuse_contribution;
layout (location = 1) out vec4 light_specular_contribution;


void main()
{
	vec2 texcoord = vec2(gl_FragCoord.x * inv_res.x, gl_FragCoord.y * inv_res.y);

	float depth = texture(depth_texture, texcoord).x;
	vec4 pos = view_projection_inverse * (2.0 * vec4(texcoord.x, texcoord.y, depth, 0.0) - vec4(1.0, 1.0, 1.0, -1.0));
	pos = pos / pos.w;

	vec4 shadow_pos = shadow_view_projection * pos;
	shadow_pos = shadow_pos / shadow_pos.w;
	shadow_pos.xyz = shadow_pos.xyz * 0.5 + vec3(0.5,0.5,0.5 + DEPTH_EPSILON);
	float shadow_depth = 0.0;
	vec3 offset = vec3(0.0, 0.0, 0.0);
	for(int y=-2; y<=2; y++) {
		for(int x=-2; x<=2; x++) {
			offset.x = x * shadowmap_texel_size.x;
			offset.y = y * shadowmap_texel_size.y;
			shadow_depth += texture(shadow_texture, shadow_pos.xyz + offset);
		}
	}
	shadow_depth = shadow_depth / 25.0;

	vec3 L = normalize(light_position - pos.xyz);
	vec3 N = (2.0 * texture(normal_texture, texcoord).xyz) - vec3(1.0, 1.0, 1.0);
	vec3 V = normalize(camera_position - pos.xyz);
	
	float specular_val = max(0.0, dot(reflect(-L, N), V));

	float r = distance(pos.xyz, light_position);
	r = light_intensity * (1 / (1 + (r * r))) * (1.0 - smoothstep(0.0, light_angle_falloff, acos(dot(-L, light_direction))));
	//r = light_intensity * (1 / (1 + (r * r))) * smoothstep(cos(light_angle_falloff), 1.0, dot(-L, light_direction));

	light_diffuse_contribution.xyz  = shadow_depth * r * light_color * clamp(max(0.0, dot(N, L)), 0.0, 1.0); 
	light_diffuse_contribution.w = 1.0;
	light_specular_contribution.xyz = shadow_depth * r * light_color * pow(specular_val, 100);
	light_specular_contribution.w = 1.0;
}
