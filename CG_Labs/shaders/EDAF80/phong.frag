#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform sampler2D diffuse_texture;
uniform sampler2D bump_texture;
uniform samplerCube cube_map;
uniform int has_textures;
uniform int texture_information;

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

    if ((texture_information & (1 << 2)) != 0) {
        mat4 TBN = mat4(vec4(normalize(fs_in.tangent), 0.0), vec4(normalize(fs_in.binormal), 0.0), vec4(N, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
        N = (TBN * vec4(texture(bump_texture, fs_in.texcoord).xyz * 2.0 - vec3(1.0, 1.0, 1.0), 0.0)).xyz;
        N = normalize(N);
    }

    //frag_color = vec4(1.0) * clamp(dot(normalize(fs_in.normal), L), 0.0, 1.0);
    frag_color.xyz = ambient;  

    if (has_textures != 0 && (texture_information & 1) != 0) {
        vec4 loadedTex =  texture(diffuse_texture, fs_in.texcoord);
        frag_color.xyz += (diffuse + loadedTex.xyz) * clamp(max(0.0, dot(N, L)), 0.0, 1.0) * 0.5;  
        frag_color.w = loadedTex.w;      
    }
    else {
        frag_color.xyz += diffuse * clamp(max(0.0, dot(N, L)), 0.0, 1.0);
        frag_color.w =  1.0;
    }

    

    if (shininess > 0.0) {
        vec3 V = normalize(camera_position - fs_in.vertex);
        float specular_val = max(0.0, dot(reflect(-L, N), V));
        if (specular_val > 0.0) {
            if ((texture_information & (1 << 1)) != 0) {
                frag_color.xyz += (specular  + texture(cube_map, reflect(-V, N)).xyz) * pow(specular_val, shininess) * 0.5;
            }
            else {
                frag_color.xyz += specular * pow(specular_val, shininess);
            }
        }
    }
}
