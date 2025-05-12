#version 330 core

// -~ Defines ~-
#define FOV 1.0

#define MAX_MARCHING_STEPS 15
#define MIN_DISTANCE 0.01
#define MAX_TOTAL_DISTANCE 10.0

#define DOUBLE_PI 6.28318530718

// -~ Uniforms ~-
uniform uvec2 Resolution;
uniform vec2 Mouse;
uniform float Time;
uniform float PeakAmp;
uniform float AvgAmp;

// -~ Output ~-
out vec4 FragColor;

// -~ Structs ~-
struct Ray {
	vec3 pos;
	vec3 dir;
	vec3 color;
};

// -~ Helpers ~-
mat2 rot2D(float angle) 
{
	float s = sin(angle);
	float c = cos(angle);
	return mat2(c, -s, s,  c);
}

float hash(vec3 p) 
{
	p = fract(p * 0.3183099 + 0.5);
	p *= 0.3183099;
	return fract(p.x + p.y + p.z);
}

float perlinNoise(vec3 p)
{
	// Determine grid cell coordinates
	vec3 i = floor(p);
	vec3 fr = fract(p);

	// Compute fade curves for interpolation
	vec3 u = fr * fr * (3.0f - 2.0f * fr);

	// Hash the corner points
	float a = hash(i);
	float b = hash(i + vec3(1.0f, 0.0f, 0.0f));
	float c = hash(i + vec3(0.0f, 1.0f, 0.0f));
	float d = hash(i + vec3(1.0f, 1.0f, 0.0f));
	float e = hash(i + vec3(0.0f, 0.0f, 1.0f));
	float f = hash(i + vec3(1.0f, 0.0f, 1.0f));
	float g = hash(i + vec3(0.0f, 1.0f, 1.0f));
	float h = hash(i + vec3(1.0f, 1.0f, 1.0f));

	// Interpolate between the corner values
	return mix(
		mix(
			mix(a, b, u.x),
			mix(c, d, u.x),
			u.y
		),
		mix(
			mix(e, f, u.x),
			mix(g, h, u.x),
			u.y
		),
		u.z
	);
}

float polarSDF(vec3 p, float radius) 
{
	float noise = perlinNoise(p * 2.0 + Time) * 0.5 + 0.5;
	return length(p) - radius * noise;
}

float march(vec3 p) 
{
	vec3 pPos = p;
	pPos.xz *= rot2D(Time);
	return polarSDF(pPos, 1.0 + PeakAmp);
}

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) 
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

// -~ Constants ~-
const vec3 color1 = vec3(0.610, 0.498, 0.650);
const vec3 color2 = vec3(0.388, 0.498, 0.350);
const vec3 color3 = vec3(0.530, 0.498, 0.620);
const vec3 color4 = vec3(3.438, 3.012, 4.025);

void main() 
{
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Set up ray
	Ray ray;
	ray.pos   = vec3(0.0, 0.0, 3.0);
	ray.dir   = normalize(vec3(uv * FOV, -1.0));
	ray.color = vec3(0.0);

	float t = 0.0;
	for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
		vec3 p = ray.pos + ray.dir * t;

		float d = march(p);
		t += d;

		ray.color = vec3(t);

		if (d <= MIN_DISTANCE)       break;
		if (t >  MAX_TOTAL_DISTANCE) break;
	}

	vec3 col = ray.color * 0.2 * palette(length(uv), color1, color2, color3, color4);
	FragColor = vec4(col, 1.0);
}
