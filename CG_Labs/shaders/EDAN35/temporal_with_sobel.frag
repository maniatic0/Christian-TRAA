#version 410

#define TIE_BREAKER_EPSILON 0.00001

uniform sampler2D depth_history_texture;
uniform sampler2D history_texture;
uniform sampler2D current_texture;

uniform sampler2D depth_texture;
uniform sampler2D velocity_texture;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;

uniform sampler2D sobel_texture;

uniform vec2 inv_res;
uniform mat4 jitter;
uniform float k_feedback_max;
uniform float k_feedback_min;

//#define COLOR_CLIP clip_aabb_ycgco
#define COLOR_CLIP clip_aabb
//#define HISTORY_CLAMPING // Use Clamping rather than clipping
#define HISTORY_COLOR_AVERAGE // Average for the color of the center of the bounding box that clips history

#define ORIGINAL_TRAA // Use inside TRAA

#ifdef ORIGINAL_TRAA
	#define BOX_3X3
#endif // ORIGINAL_TRAA

//#define BOX_5X5

#ifdef BOX_3X3

#undef BOX_4X4
#undef BOX_5X5 

#define BOX_RANGE (1.0)
#define BOX_AMOUNT (9.0)

#endif // BOX_3X3

#ifdef BOX_4X4

#undef BOX_3X3
#undef BOX_5X5 

#define BOX_RANGE (1.5)
#define BOX_AMOUNT (16.0)

#endif // BOX_4X4

#ifdef BOX_5X5

#undef BOX_3X3
#undef BOX_4X4 

#define BOX_RANGE (2)
#define BOX_AMOUNT (25.0)

#endif // BOX_5X5

uniform mat4 rgb_ycgco = mat4(
	0.25, -0.25, 0.5, 0.0,
	0.5, 0.5, 0.0, 0.0,
	0.25, -0.25, -0.5, 0.0,
	0.0, 0.0, 0.0, 1.0
);

uniform mat4 ycgco_rgb = mat4(
	1.0, 1.0, 1.0, 0.0,
	-1.0, 1.0, -1.0, 0.0,
	1.0, 0.0, -1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
);

in VS_OUT {
	vec2 texcoord;
} fs_in;

layout (location = 0) out vec4 current_history_texture;
layout (location = 1) out vec4 temporal_output;
layout (location = 2) out float depth_next_history_output;

// Convert from RGB to YCgCo
vec4 rgb_to_ycgco(vec4 color_in) {
	return rgb_ycgco * color_in;
}

// Convert from YCgCo to RGB
vec4 ycgco_to_rgb (vec4 color_in) {
	return ycgco_rgb * color_in;
}

// Taken from http://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in
// note: clips towards aabb center + p.w
vec4 clip_aabb( vec3 aabb_min, // cn_min
	vec3 aabb_max, // cn_max
	vec4 p, // c_in’
	vec4 q) // c_hist
{
	vec3 p_clip = 0.5 * (aabb_max + aabb_min);
	vec3 e_clip = 0.5 * (aabb_max - aabb_min);
	vec4 v_clip = q - vec4(p_clip, p.w);
	vec3 v_unit = v_clip.xyz / e_clip;
	vec3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));
	//float ma_unit = length(v_clip.xyz) / length(e_clip.xyz);
	if (ma_unit > 1.0)
		return vec4(p_clip, p.w) + v_clip / ma_unit;
	else
		return q;// point inside aabb
}

// Clip in YCgCo space
vec4 clip_aabb_ycgco( vec3 aabb_min, // cn_min
	vec3 aabb_max, // cn_max
	vec4 p, // c_in’
	vec4 q) // c_hist
{
	return ycgco_to_rgb(clip_aabb(
		rgb_to_ycgco(vec4(aabb_min, 1.0)).rgb,
		rgb_to_ycgco(vec4(aabb_max, 1.0)).rgb,
		rgb_to_ycgco(p),
		rgb_to_ycgco(q))
	);
}

float luminance(vec4 color_in) {
	return 0.25 * color_in.r + 0.5 * color_in.g + 0.25 * color_in.b;
}

float luminance(sampler2D texture_in, vec2 uv, float bias) {
	vec4 sample_color = texture(texture_in, uv, bias);
	return luminance(sample_color);
}

float luminance(sampler2D texture_in, vec2 uv) {
	return luminance(texture_in, uv, 0.0);
}

void main()
{
	vec4 j_uv; // Jitter Pos
	j_uv = vec4(fs_in.texcoord, 0.0, 0.0);
	j_uv = jitter * (2.0 * j_uv - vec4(1.0, 1.0, 0.0, -1.0));
	j_uv = j_uv * 0.5 + vec4(0.5);


	depth_next_history_output = texture(depth_texture, j_uv.xy).x; // Save depth history for next cycle

	vec2 p_uv;
	vec4 p = vec4(1.0, 1.0, 1.0, 0.0); // everything at the back of fustrum
	float depth;
	float closest_depth;
	float test_step;
	// Color constraints
	vec4 cn_max = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 cn_min = vec4(1.0, 1.0, 1.0, 0.0);
	vec4 cn_temp;
	vec4 cn_avg = vec4(0.0);

	float sobel = texture(sobel_texture, j_uv.xy).r;
	float sobel_avg = 0.0;
	float sobel_step;
	float highest_sobel;

	// Neighbours box
	for(float y=-BOX_RANGE; y<=BOX_RANGE; y+=1.0) {
		for(float x=-BOX_RANGE; x<=BOX_RANGE; x+=1.0) {
			p_uv = j_uv.xy + vec2(x * inv_res.x, y * inv_res.y);

			depth = texture(depth_texture, p_uv).x;
			sobel = texture(sobel_texture, p_uv).x;

			//sobel_step = step(p.w + TIE_BREAKER_EPSILON, sobel);

			// test_step = step(p.z, depth + TIE_BREAKER_EPSILON); // closest pixel

			// Test look for the pixel with the highest sobel value
			test_step =  step(p.w + TIE_BREAKER_EPSILON, sobel);

			p.xyzw = test_step * p.xyzw +  (1.0 - test_step) * vec4(p_uv.x, p_uv.y, depth, sobel); 

			sobel_avg += sobel;

			// Color Constraint
			cn_temp = texture(current_texture, p_uv);
			cn_min = min(cn_min, cn_temp);
			cn_max = max(cn_max, cn_temp);
			cn_avg += cn_temp;
		}
	}

	cn_avg = cn_avg / BOX_AMOUNT;
	sobel_avg = sobel_avg / BOX_AMOUNT;

	p_uv = p.xy;
	closest_depth = p.z;
	sobel = p.w;

	// Velocity history
	//vec2 v = texture(velocity_texture, p_uv).rg; // calculated by test above
	vec2 v = texture(velocity_texture, j_uv.xy).rg; // this exact pixel
	vec2 q_uv = (2.0 * fs_in.texcoord - vec2(1.0)) - v;
	q_uv = 0.5 * q_uv + vec2(0.5);

	vec4 c_hist = texture(history_texture, q_uv);

	// Color Constraint Cross
	vec4 cn_cross_max = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 cn_cross_min = vec4(1.0, 1.0, 1.0, 0.0);
	vec4 cn_cross_avg = vec4(0.0);
	float sobel_cross_avg = 0.0;

	vec2 cross_temp;

	cross_temp = j_uv.xy + vec2((-1.0) * inv_res.x, 0.0);
	cn_temp = texture(current_texture, cross_temp);
	cn_cross_min = min(cn_cross_min, cn_temp);
	cn_cross_max = max(cn_cross_max, cn_temp);
	cn_cross_avg += cn_temp;
	sobel_cross_avg +=  texture(sobel_texture, cross_temp).r;

	cross_temp = j_uv.xy + vec2(0.0, (1.0) * inv_res.y);
	cn_temp = texture(current_texture, cross_temp);
	cn_cross_min = min(cn_cross_min, cn_temp);
	cn_cross_max = max(cn_cross_max, cn_temp);
	cn_cross_avg += cn_temp;
	sobel_cross_avg +=  texture(sobel_texture, cross_temp).r;

	cn_temp = texture(current_texture, j_uv.xy);
	cn_cross_min = min(cn_cross_min, cn_temp);
	cn_cross_max = max(cn_cross_max, cn_temp);
	cn_cross_avg += cn_temp;
	sobel_cross_avg +=  texture(sobel_texture, cross_temp).r;

	cross_temp = j_uv.xy + vec2(0.0, (-1.0) * inv_res.y);
	cn_temp = texture(current_texture, cross_temp);
	cn_cross_min = min(cn_cross_min, cn_temp);
	cn_cross_max = max(cn_cross_max, cn_temp);
	cn_cross_avg += cn_temp;
	sobel_cross_avg +=  texture(sobel_texture, cross_temp).r;

	cross_temp = j_uv.xy + vec2((1.0) * inv_res.x, 0.0);
	cn_temp = texture(current_texture, cross_temp);
	cn_cross_min = min(cn_cross_min, cn_temp);
	cn_cross_max = max(cn_cross_max, cn_temp);
	cn_cross_avg += cn_temp;
	sobel_cross_avg +=  texture(sobel_texture, cross_temp).r;

	cn_cross_avg /= 5.0;
	sobel_cross_avg /= 5.0;

	// Mix min-max averaging

	cn_min = mix(cn_min, cn_cross_min, 0.5);
	cn_max = mix(cn_max, cn_cross_max, 0.5);
	cn_avg = mix(cn_avg, cn_cross_avg, 0.5);
	sobel_avg = mix(sobel_avg, sobel_cross_avg, 0.5);


	//float speed = length(texture(velocity_texture, j_uv.xy).rg);
	//float specular_part = luminance(texture(specular_texture, j_uv.xy).rgba);
	
	//sobel = clamp(mix(sobel_avg, sobel, 0.5), 0.3, 1.0);

	sobel = 1.0;

	vec4 c_in = texture(current_texture, j_uv.xy);
	cn_min = mix(c_in, cn_min, sobel);
	cn_max = mix(c_in, cn_max, sobel);

#ifdef HISTORY_CLAMPING
	vec4 c_hist_constrained = clamp(c_hist, cn_min, cn_max);
#elif defined(HISTORY_COLOR_AVERAGE)
	vec4 c_hist_constrained = COLOR_CLIP(cn_min.rgb, cn_max.rgb, clamp(cn_avg, cn_min, cn_max), c_hist);
#else
	vec4 c_hist_constrained = COLOR_CLIP(cn_min.rgb, cn_max.rgb, c_in, c_hist);
#endif // HISTORY_COLOR_AVERAGE

	// Feedback Luminance Weighting
	float lum_current = luminance(c_in);
	float lum_history = luminance(c_hist_constrained);

	float unbiased_diff = abs(lum_current - lum_history) / max(lum_current, max(lum_history, 0.2));
	float unbiased_weight = 1.0 - unbiased_diff;
	float unbiased_weight_sqr = unbiased_weight * unbiased_weight;
	float k_feedback = mix(k_feedback_min, k_feedback_max, unbiased_weight_sqr);

	current_history_texture = mix(c_in, c_hist_constrained, k_feedback); // Inside TRAA
	temporal_output.xyzw = current_history_texture.xyzw;
}