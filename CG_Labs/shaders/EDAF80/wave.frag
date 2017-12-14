#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec4 color_deep;
uniform vec4 color_shallow;
uniform float actual_time;

uniform mat4 normal_model_to_world;

uniform samplerCube cube_map;
uniform sampler2D bump_texture;

in VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec2 texcoord;
    vec3 tangent;
    vec3 binormal;
	float diffx;
	float diffz;
} fs_in;

out vec4 frag_color;


void main()
{

	vec3 N;
	vec3 L = normalize(light_position - fs_in.vertex);
	vec3 V = normalize(camera_position - fs_in.vertex);
	
    N = vec3(-fs_in.diffx, 1.0, -fs_in.diffz);
    N = normalize(N);
    
    vec2 texScale = vec2(8.0, 4.0);
    float bumpTime = mod(actual_time, 100.0);
    vec2 bumpSpeed =  vec2(-0.05, 0);
    
    vec2 bumpCoord0 = fs_in.texcoord*texScale + bumpTime*bumpSpeed;
	vec2 bumpCoord1 = fs_in.texcoord*texScale*2 + bumpTime*bumpSpeed*4;
	vec2 bumpCoord2 = fs_in.texcoord*texScale*4 + bumpTime*bumpSpeed*8;
    
    vec3 tangent = vec3(0.0, fs_in.diffz, 1.0);
    vec3 binormial = vec3(1.0, fs_in.diffx, 0.0);
    
    mat4 TNB = mat4(vec4(normalize(fs_in.tangent), 0.0), vec4(normalize(fs_in.normal), 0.0), vec4(normalize(fs_in.binormal), 0.0), vec4(0.0, 0.0, 0.0, 1.0));
   	mat4 BTN = mat4(vec4(normalize(binormial), 0.0), vec4(normalize(tangent), 0.0), vec4(N, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
	
	N = texture(bump_texture, bumpCoord0).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
	N += texture(bump_texture, bumpCoord1).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
	N += texture(bump_texture, bumpCoord2).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
   	N = normalize(N);
   	N = (TNB * BTN * vec4(N, 0.0)).xyz;
    N = normalize(N);
    
    
    float facing = 1 - max(dot(V, N), 0.0);
    
    float fastFresnel = 0.02037 + (1 - 0.02037) * pow(1 - dot(V, N),5.0);
    
    frag_color = texture(cube_map, reflect(-V, N)) * fastFresnel 
    + texture(cube_map, refract(-V, N, 1/1.33)) * (1.0 - fastFresnel);
	
	frag_color += mix(color_deep, color_shallow, facing);
	
}
