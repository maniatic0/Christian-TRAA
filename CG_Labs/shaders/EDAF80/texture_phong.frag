#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 specular;
uniform float shininess;

uniform sampler2D diffuse_texture;

uniform mat4 normal_model_to_world;

in VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec2 texcoord;
    vec3 tangent;
    vec3 binormal;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 L = normalize(light_position - fs_in.vertex);
    vec3 N = normalize(fs_in.normal);
    vec3 V = normalize(camera_position - fs_in.vertex);
    frag_color.xyz = ambient;  

    vec4 loadedTex =  texture(diffuse_texture, fs_in.texcoord);
    frag_color.xyz += loadedTex.xyz * clamp(max(0.0, dot(N, L)), 0.0, 1.0);  
    frag_color.w = loadedTex.w;    

    frag_color.xyz += specular * pow(max(0.0, dot(reflect(-L, N), V)), max(shininess, 0.0));
}
