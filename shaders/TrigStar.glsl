#version 330 core

// -~ Defines ~-
#define DOUBLE_PI 6.28318530718

// -~ Uniforms ~-
uniform uvec2 Resolution;
uniform vec2 Mouse;
uniform float Time;
uniform float PeakAmp;
uniform float AvgAmp;
uniform vec3	Color1;
uniform vec3	Color2;
uniform vec3	Color3;
uniform vec3	Color4;

// -~ Output ~-
out vec4 FragColor;

// -~ Helpers ~-
float sdEquilateralTriangle(vec2 p, float r) // From: https://iquilezles.org/articles/distfunctions2d/
{
	const float k = sqrt(3.0);
	p.x = abs(p.x) - r;
	p.y = p.y + r/k;
	if (p.x + k*p.y > 0.0)
		p = vec2(p.x - k*p.y, -k*p.x - p.y) * 0.5;
	p.x -= clamp(p.x, -2.0*r, 0.0);
	return -length(p) * sign(p.y);
}

// From: https://iquilezles.org/articles/palettes/
vec3 palette(float t, vec3 a, vec3 b, vec3 c, vec3 d)
{
	return a + b * cos(DOUBLE_PI * (c * t + d));
}

void main()
{
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	vec2 uv0 = uv;
	vec3 finalColor = vec3(0.0);

	for (int i = 0; i < 4; i++)
	{
		uv = fract(uv * 2.0) - 0.5;
		vec3 color = vec3(sdEquilateralTriangle(uv, 0.2) * exp(-length(uv0)));
		color = sin(color * 10.0 + (1.0 - AvgAmp) * 6.0) / 10.0;
		color = abs(color);
		color = smoothstep(0.0, 0.1, color);
		color = pow(0.1 / color, vec3(1.3));
		finalColor = color * palette(length(uv0) + float(i) * (1.0 - PeakAmp),
					    Color1, Color2, Color3, Color4);
	}

	FragColor = vec4(finalColor, 1.0);
}
