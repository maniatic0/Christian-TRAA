#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float time;

void main()
{
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(((sin(time) + 1.0)/2.0) * vertex, 1.0);
}



