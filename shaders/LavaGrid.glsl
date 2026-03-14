#version 330 core

// -~ Uniforms ~-
uniform uvec2	Resolution;
uniform vec2	Mouse;
uniform float	Time;
uniform float	PeakAmp;
uniform float	AvgAmp; // Smoother
uniform vec3	Color1;
uniform vec3	Color2;
uniform vec3	Color3;
uniform vec3	Color4;

// -~ Defs ~-
#define DOUBLE_PI 6.28318530718
#define SPACE_REP 10.0
#define CIRCLE_R 0.4
#define CIRCLE_ANTI_ALIS 0.07
#define CHANNEL_INTENSITY 0.7
#define SPEED 0.4

// -~ Output ~-
out vec4 FragColor;

float circleSdf(in vec2 uv, in float r)
{
	return length(uv) - r;
}

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d)
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

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

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;

	vec2 uv_orig = uv * SPACE_REP;
	uv = fract(uv_orig);
	uv -= vec2(0.5);

	float n = noise(uv_orig + Time * SPEED) + PeakAmp * 0.2;

	// Each circle on a sparate channel
	vec3 circle1 = vec3(smoothstep(0.0, CIRCLE_ANTI_ALIS, circleSdf(uv * vec2(sin(Time), cos(Time * 0.5)), CIRCLE_R))) * vec3(CHANNEL_INTENSITY, 0.0, 0.0);
	vec3 circle2 = vec3(smoothstep(0.0, CIRCLE_ANTI_ALIS, circleSdf(uv * vec2(sin(Time), cos(Time)), CIRCLE_R))) * vec3(0.0, CHANNEL_INTENSITY, 0.0);
	vec3 circle3 = vec3(smoothstep(0.0, CIRCLE_ANTI_ALIS, circleSdf(uv * vec2(sin(Time * 0.5), cos(Time * 0.5)), CIRCLE_R * AvgAmp * n * 3.0))) * 
			vec3(0.0, 0.0, CHANNEL_INTENSITY);

	vec3 color = palette((circle1.r + circle2.g + circle3.b), Color1, Color2, Color3, Color4);
	FragColor = vec4(color, 1.0); 
}
