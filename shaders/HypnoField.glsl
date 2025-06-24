#version 330 core

// -~ Defines ~-
#define DOUBLE_PI 6.28318530718

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

// -~ Output ~-
out vec4 FragColor;

// -~ Helpers ~-
mat2 rot2D(in float angle)
{
        float s = sin(angle);
        float c = cos(angle);
        return mat2(c, -s, s, c);
}

float circleSDF(in vec2 uv, float radius)
{
	return length(uv) - radius;
}

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d)
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

float smin(float a, float b, float k)
{
        k *= 1.0;
        float r = exp2(-a/k) + exp2(-b/k);
        return -k*log2(r);
}

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;

	uv *= 0.6;
	mouse *= 0.35;

	float c1 = circleSDF(uv, 0.4);
	
	mat2 rot = rot2D(Time);

	float c2 = circleSDF((uv * rot) + 0.1 + (AvgAmp * 0.7), 0.3);

	float scene = max(c1, c2) - 2.0 * smin(c1, c2, 0.2);
	scene = circleSDF((uv - mouse) * rot, fract(scene * 11.0) * PeakAmp);
	if (scene < 0.001) scene = abs((uv.x + uv.y - AvgAmp * 2.0) * scene) + 0.4;
	vec3 color = vec3(scene) * palette(scene + (AvgAmp * 2.0), Color1, Color2, Color3, Color4);
	FragColor = vec4(color, 1.0);
}
