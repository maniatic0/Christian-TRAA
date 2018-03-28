#version 410

uniform sampler2D temporal_output;

uniform vec2 inv_res;

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 sharpen_output;


#define USE_SHARPEN
#define KERNEL_SIZE 5
//#define USE_UNCHARTED_4_SATURATE

// Note: the center pixel is always added so its not necesarry to add one to the center of the convolution
#if KERNEL_SIZE == 5

// Laplacian Filtering
uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](4.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0));

#ifdef USE_UNCHARTED_4_SATURATE
	#define NORMALIZATION_CONSTANT (1.0)
#else
	#define NORMALIZATION_CONSTANT (1.0/4.0)
#endif // USE_UNCHARTED_4_SATURATE

#elif KERNEL_SIZE == 9

uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](8.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](
	vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0), 
	vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));

#define NORMALIZATION_CONSTANT (1.0/8.0)

#elif KERNEL_SIZE == 25

uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](8.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
	-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](
	vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0), 
	vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
	vec2(-2.0, -2.0), vec2(-2.0, -1.0), vec2(-2.0, 0.0), vec2(-2.0, 1.0), vec2(-2.0, 2.0),
	vec2(2.0, -2.0), vec2(2.0, -1.0), vec2(2.0, 0.0), vec2(2.0, 1.0), vec2(2.0, 2.0),
	vec2(-1.0, -2.0), vec2(0.0, -2.0), vec2(1.0, -2.0),
	vec2(-1.0, 2.0), vec2(0.0, 2.0), vec2(1.0, 2.0));

#define NORMALIZATION_CONSTANT (1.0/16.0)

#endif

void main()
{
	#ifdef USE_SHARPEN
	sharpen_output.xyz = vec3(0.0);
	for(int i = 0; i < KERNEL_SIZE; i++) {
		sharpen_output += kernel[i] * texture(temporal_output, fs_in.texcoord + (inv_res * kernel_off[i]));
	}
	sharpen_output *= NORMALIZATION_CONSTANT;
	sharpen_output += texture(temporal_output, fs_in.texcoord);
	#else
	sharpen_output = texture(temporal_output, fs_in.texcoord);
	#endif // USE_SHARPEN
	sharpen_output.w = 1.0;
}
