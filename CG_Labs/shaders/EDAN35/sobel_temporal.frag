#version 410

#define TIE_BREAKER_EPSILON 0.00001

uniform sampler2D sobel_current_texture;
uniform sampler2D sobel_history_texture;
uniform sampler2D velocity_texture;

uniform vec2 inv_res;
uniform mat4 jitter;
uniform float k_feedback_max;
uniform float k_feedback_min;

#define BOX_RANGE (1.0)
#define BOX_AMOUNT (9.0)


in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out float current_sobel_history_texture;


void main()
{
	vec4 j_uv; // Jitter Pos
	j_uv = vec4(fs_in.texcoord, 0.0, 0.0);
	j_uv = jitter * (2.0 * j_uv - vec4(1.0, 1.0, 0.0, -1.0));
	j_uv = j_uv * 0.5 + vec4(0.5);

	vec2 p_uv;

	float sobel_temp;
	float sobel_min = 1.0;
	float sobel_max = 0.0;

	// Neighbours box
	for(float y=-BOX_RANGE; y<=BOX_RANGE; y+=1.0) {
		for(float x=-BOX_RANGE; x<=BOX_RANGE; x+=1.0) {
			p_uv = j_uv.xy + vec2(x * inv_res.x, y * inv_res.y);

			sobel_temp = texture(sobel_current_texture, p_uv).x;

			sobel_min = min(sobel_min, sobel_temp);
			sobel_max = max(sobel_max, sobel_temp);
		}
	}

	// Velocity history
	vec2 v = texture(velocity_texture, j_uv.xy).rg; // this exact pixel
	vec2 q_uv = (2.0 * fs_in.texcoord - vec2(1.0)) - v;
	q_uv = 0.5 * q_uv + vec2(0.5);

	float sobel_hist = texture(sobel_history_texture, q_uv).x;

	// Sobel Constraint Cross
	float sobel_cross_min = 1.0;
	float sobel_cross_max = 0.0;

	vec2 cross_temp;

	cross_temp = j_uv.xy + vec2((-1.0) * inv_res.x, 0.0);
	sobel_temp = texture(sobel_current_texture, cross_temp).x;
	sobel_cross_min = min(sobel_cross_min, sobel_temp);
	sobel_cross_max = max(sobel_cross_max, sobel_temp);

	cross_temp = j_uv.xy + vec2(0.0, (1.0) * inv_res.y);
	sobel_temp = texture(sobel_current_texture, cross_temp).x;
	sobel_cross_min = min(sobel_cross_min, sobel_temp);
	sobel_cross_max = max(sobel_cross_max, sobel_temp);

	sobel_temp = texture(sobel_current_texture, j_uv.xy).x;
	sobel_cross_min = min(sobel_cross_min, sobel_temp);
	sobel_cross_max = max(sobel_cross_max, sobel_temp);

	cross_temp = j_uv.xy + vec2(0.0, (-1.0) * inv_res.y);
	sobel_temp = texture(sobel_current_texture, cross_temp).x;
	sobel_cross_min = min(sobel_cross_min, sobel_temp);
	sobel_cross_max = max(sobel_cross_max, sobel_temp);

	cross_temp = j_uv.xy + vec2((1.0) * inv_res.x, 0.0);
	sobel_temp = texture(sobel_current_texture, cross_temp).x;
	sobel_cross_min = min(sobel_cross_min, sobel_temp);
	sobel_cross_max = max(sobel_cross_max, sobel_temp);

	// Mix min-max averaging

	sobel_min = mix(sobel_min, sobel_cross_min, 0.5);
	sobel_max = mix(sobel_max, sobel_cross_max, 0.5);

	float sobel_in = texture(sobel_current_texture, j_uv.xy).x;


	float sobel_hist_constrained = clamp(sobel_hist, sobel_min, sobel_max);

	// Feedback Luminance Weighting

	float unbiased_diff = abs(sobel_in - sobel_hist) / max(sobel_in, max(sobel_hist, 0.2));
	float unbiased_weight = 1.0 - unbiased_diff;
	float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
	float k_feedback = mix(k_feedback_min, k_feedback_max, unbiased_weight_sqr);

	current_sobel_history_texture = mix(sobel_in, sobel_hist_constrained, k_feedback); 
}