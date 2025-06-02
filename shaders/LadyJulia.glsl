#version 330 core

// -~ Defines ~-
#define MAX_ITR 50
#define MAX_LIM 20
#define DOUBLE_PI 6.28318530718
#define AMP_SCALE 12.0

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

// -~ Helpers ~-

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d)
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
	mouse.y = -mouse.y;

	float a = uv.x;
	float b = uv.y;

	int i;
	for (i = 0; i < MAX_ITR; i++) {
		float aa = a * a - b * b;
		float bb = 2 * a * b;
		a = aa + mouse.x * 0.6;
		b = bb + mouse.y * 0.6;

		if (abs(a + b + PeakAmp * AMP_SCALE) > MAX_LIM) break;
	}

	vec3 color;
	if (i != MAX_ITR) {
		float dist = i / MAX_ITR;
		color = vec3(1.0) ;
		color *= palette(dist + AvgAmp * 0.5 * float(i), Color1, Color2, Color3, Color4);
	} else {
		color = vec3(0.0);
	}

	FragColor = vec4(color, 1.0);
}
