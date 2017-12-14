#version 410

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// Similarly, normal is set to location 1, which corresponds to attribute 1 of
// the vertex array, and therefore will be filled with normals taken out of our
// buffer.
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float actual_time;

uniform float amplitude1;
uniform float frequency1;
uniform float phase1;
uniform float sharpness1;
uniform vec2 direction1;

uniform float amplitude2;
uniform float frequency2;
uniform float phase2;
uniform float sharpness2;
uniform vec2 direction2;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {

	vec3 vertex;
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	float diffx;
	float diffz;
} vs_out;


void main()
{
	vec3 vert = vertex;

	float aux = sin((direction1.x * vertex.x + direction1.y * vertex.z) 
	* frequency1 + actual_time * phase1) * 0.5f + 0.5f;
	
	float aux_diff = cos((direction1.x * vertex.x + direction1.y * vertex.z) * frequency1 + actual_time * phase1);
	
	vert.y = vert.y + amplitude1 * pow(aux, sharpness1);
	
	aux_diff = 0.5 * sharpness1 * frequency1 * amplitude1 * pow(aux, sharpness1 - 1.0) * aux_diff;
	
	vs_out.diffx = aux_diff  * direction1.x;
	
	vs_out.diffz = aux_diff * direction1.y;
	
	aux = sin((direction2.x * vertex.x + direction2.y * vertex.z) 
	* frequency2 + actual_time * phase2) * 0.5f + 0.5f;
	
	aux_diff = cos((direction2.x * vertex.x + direction2.y * vertex.z) * frequency2 + actual_time * phase2);
	
	vert.y += amplitude2 * pow(aux, sharpness2);
	
	aux_diff = 0.5 * sharpness2 * frequency2 * amplitude2 * pow(aux, sharpness2 - 1.0) * aux_diff;
	
	vs_out.diffx += aux_diff  * direction1.x;
	
	vs_out.diffz += aux_diff * direction1.y;
	
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vert, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.tangent = vec3(normal_model_to_world * vec4(tangent, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(binormal, 0.0));

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vert, 1.0);
}

