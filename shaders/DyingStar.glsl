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
#define SPACE_SCALE 16.0
#define NOISE_SCALE 128.0
#define NOISE_INTENSITY 8.0
#define HOLE_INNER 1.6
#define STAR_RAD 0.084

// -~ Output ~-
out vec4 FragColor;

// -~ Helpers ~-

float dot2(in vec2 v)
{
	return dot(v, v);
}

// Not an SDF
float dimondstar(in vec2 uv, in float r, in float intensity)
{
	const float t = 0.23;
	uv /= r;
	uv = abs(uv) - 0.5;
	float d  = dot2(uv);
	return clamp(((t - d) * step(uv.x, t * t) * step(uv.y, t * t)) * -intensity, 0.0, 1.0);
}

mat2 rot2D(float angle) 
{
	float s = sin(angle);
	float c = cos(angle);
	return mat2(c, -s, s,  c);
}

// from book of shaders
vec2 random2 (vec2 uv)
{
	uv = vec2( dot(uv,vec2(127.1,311.7)),
		dot(uv,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(uv)*43758.5453123);
}

// Gradient Noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/XdXGW8
float noise(in vec2 uv)
{
	vec2 i = floor(uv);
	vec2 f = fract(uv);

	vec2 u = f*f*(3.0-2.0*f);

	return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
		     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
		mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
		     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

// From Lygia
float rand(in float x)
{
	x = fract(x * 443.897);
	x *= x + 33.3;
	x *= x+x;
	return fract(x);
}

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;

	float uv_len = length(uv); // Calculating once to use everywhere

	vec2 uv_orig = uv;
	float uv_len_fract = uv_len * 10;
	uv *= rot2D(Time * (rand(floor(uv_len_fract))));

	float n = noise((uv * SPACE_SCALE) + (NOISE_SCALE * uv_len)) * NOISE_INTENSITY;

	float hole = smoothstep(-1.5, (0.4*(AvgAmp+ 1.5)/uv_len), n - (uv_len * (0.75/(AvgAmp + 0.06))));

	uv_len_fract*= 1.14;
	float smoothness = 1.0 - (smoothstep(0.7, 0.0, fract(uv_len_fract)) * smoothstep(0.0, 0.7, fract(uv_len_fract)));
	smoothness = smoothstep(0.65,1.0, smoothness);

	vec3 color = (vec3(hole) * smoothness * 
		     smoothstep(0.0, 0.3, (uv_len * SPACE_SCALE) - HOLE_INNER)) +
		     dimondstar(uv_orig * vec2(1.0, 0.7 - AvgAmp*0.1), STAR_RAD + AvgAmp*0.1, 4.0 + PeakAmp*0.5);

	// color variation
	color = (color.x > AvgAmp*0.1) ? color*2.0 : color;

	FragColor = vec4(color, 1.0);
}
