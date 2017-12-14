#version 410

uniform sampler2D temporal_output;

uniform vec2 inv_res;

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 sharpen_output;


#define KERNEL_SIZE 5

// Note: the center pixel is always added so its not necesarry to add one to the center of the convolution
#if KERNEL_SIZE == 5

uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](4.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0));

#elif KERNEL_SIZE == 9

uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](8.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](
	vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0), 
	vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0));

#elif KERNEL_SIZE == 25

uniform float kernel[KERNEL_SIZE] = float[KERNEL_SIZE](7.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
	-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0);
uniform vec2 kernel_off[KERNEL_SIZE] = vec2[KERNEL_SIZE](
	vec2(0.0, 0.0), vec2(-1.0, 0.0), vec2(1.0, 0.0), vec2(0.0, -1.0), vec2(0.0, 1.0), 
	vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
	vec2(-2.0, -2.0), vec2(-2.0, -1.0), vec2(-2.0, 0.0), vec2(-2.0, 1.0), vec2(-2.0, 2.0),
	vec2(2.0, -2.0), vec2(2.0, -1.0), vec2(2.0, 0.0), vec2(2.0, 1.0), vec2(2.0, 2.0),
	vec2(-1.0, -2.0), vec2(0.0, -2.0), vec2(1.0, -2.0),
	vec2(-1.0, 2.0), vec2(0.0, 2.0), vec2(1.0, 2.0));

#endif

void main()
{
	sharpen_output = texture(temporal_output, fs_in.texcoord);
	for(int i = 0; i < KERNEL_SIZE; i++) {
		sharpen_output += kernel[i] * texture(temporal_output, fs_in.texcoord + (inv_res * kernel_off[i]));
	}
	sharpen_output = clamp(sharpen_output, 0.0, 1.0);
	sharpen_output.w = 1.0;
}
