#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

uniform samplerCube cube_map;
uniform int has_textures;

in VS_OUT {
    vec3 vertex;
    vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
    frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    if (has_textures != 0){
        vec3 N = normalize(fs_in.normal);
        vec3 V = normalize(camera_position - fs_in.vertex);
        frag_color += texture(cube_map, reflect(V, N));
    }
}
