#version 410

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform mat4 old_MVP;
uniform mat4 unjittered_vertex_world_to_clip;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

out VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec4 current_pos;
	vec4 old_pos;
} vs_out;


void main() {
	vs_out.normal   = normalize(normal);
	vs_out.texcoord = texcoord.xy;
	vs_out.tangent  = normalize(tangent);
	vs_out.binormal = normalize(binormal);

	vec4 world_pos = vertex_model_to_world * vec4(vertex, 1.0);
	vs_out.current_pos = unjittered_vertex_world_to_clip * world_pos;
	vs_out.old_pos = old_MVP * vec4(vertex, 1.0);
	gl_Position = vertex_world_to_clip * world_pos;
}
