#version 330 core

// -~ Defines ~-
#define FOV 1.0
#define MAX_MARCHING_STEPS 30
#define MIN_DISTANCE 0.01
#define MAX_TOTAL_DISTANCE 10.0
#define DOUBLE_PI 6.28318530718

// -~ Uniforms ~-
uniform uvec2	Resolution;
uniform vec2	Mouse;
uniform float	Time;
uniform float	PeakAmp;
uniform float	AvgAmp;
uniform vec3	Color1;
uniform vec3	Color2;
uniform vec3	Color3;
uniform vec3	Color4;

// -~ Output ~-
out vec4 FragColor;

// -~ Structs ~-
struct Ray {
	vec3 pos;
	vec3 dir;
};

// -~ Helpers ~-
// from book of shaders
vec2 random2 (vec2 uv)
{
	uv = vec2( dot(uv,vec2(127.1,311.7)),
		dot(uv,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(uv)*43758.5453123);
}

// Gradient Noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/XdXGW8
float noise(vec2 uv)
{
	vec2 i = floor(uv);
	vec2 f = fract(uv);

	vec2 u = f*f*(3.0-2.0*f);

	return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
		     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
		mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
		     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

float perlinPlane(vec3 p, float y)
{
	float noise1 = noise(p.xz + Time);
	float noise2 = noise(p.xz) * 3.0 * PeakAmp;
	return p.y - y + (noise1 * noise2);
}

float march(vec3 p)
{
	return perlinPlane(p, -1.4);
}

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d) 
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
	mouse.y = -mouse.y;

	// Raymarching
	Ray ray;
	ray.pos   = vec3(0.0, 0.0, 3.0);
	ray.dir   = normalize(vec3(uv * FOV, -1.0));

	float t = 0.0;
	vec3 p;
	int i = 0;
	for (; i < MAX_MARCHING_STEPS; i++) {
		p = ray.pos + ray.dir * t;

		float d = march(p);
		t += d;


		if (d <= MIN_DISTANCE)       break;
		if (t >  MAX_TOTAL_DISTANCE) break;
	}

	vec3 col;
	if (t > MAX_TOTAL_DISTANCE) {
		vec2 p = uv;
		p.y *=1.4;
		col = vec3(length(p) * AvgAmp) * Color2;
		col *= 0.3;
	} else {
		col = palette(length(p * (1.0 - AvgAmp) * 0.3) + float(i) * 0.03, Color1, Color2, Color3, Color4);
		col *= (MAX_TOTAL_DISTANCE - t) * 0.14;
	}

	FragColor = vec4(col, 1.0);
}
