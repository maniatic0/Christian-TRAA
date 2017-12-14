#version 410

uniform sampler2D temporal_output;
uniform sampler2D velocity_texture;

uniform vec2 inv_res;
uniform float current_fps;
uniform mat4 jitter;
#define TARGET_FPS 25.0

#define MAX_SAMPLES 2000

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

#define MOTION_BLUR

void main()
{
	vec4 j_uv; // Jitter Pos
	j_uv = vec4(fs_in.texcoord, 0.0, 0.0);
	//j_uv = jitter * (2.0 * j_uv - vec4(1.0, 1.0, 0.0, -1.0));
	//j_uv = j_uv * 0.5 + vec4(0.5);

	frag_color = texture(temporal_output, j_uv.xy);
	#ifdef MOTION_BLUR
	vec2 velocity = texture(velocity_texture, j_uv.xy).rg;

	velocity *= current_fps / TARGET_FPS;

	float speed = length(vec2(velocity.x * inv_res.x, velocity.y * inv_res.y));
	int nSamples = clamp(int(speed), 1, MAX_SAMPLES);

	for (int i = 1; i < nSamples; ++i) {
		vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5);
		vec2 pos = (2.0 * j_uv.xy - vec2(1.0)) + offset;
		pos = 0.5 * pos + vec2(0.5);
		frag_color += texture(temporal_output, pos);
	}
	frag_color /= float(nSamples);
	#endif
}
