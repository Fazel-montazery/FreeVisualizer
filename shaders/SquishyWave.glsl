#version 330 core

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

// -~ Defines ~-
#define LINE_THK 0.009
#define DOUBLE_PI 6.28318530718

// -~ Output ~-
out vec4 FragColor;

// -~ Helpers ~-
// From https://www.flong.com/archive/texts/code/shapers_exp/
float blinnWyvillApx(in float x)
{
        float x2 = x * x;
        float x4 = x2 * x2;
        float x6 = x2 * x4;

        const float fa = (4.0 / 9.0);
        const float fb = (17.0 / 9.0);
        const float fc = (22.0 / 9.0);

        return (fa * x6) - (fb * x4) + (fc * x2);
}

float plot(in vec2 uv, in float f)
{
        return  smoothstep( f - LINE_THK, f, uv.y) -
                smoothstep( f, f + LINE_THK, uv.y);
}

float waves(in vec2 uv)
{
	float w1 = plot(uv + AvgAmp * 2.2, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 2.2)));
	float w2 = plot(uv + AvgAmp * 1.9, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 1.9)));
	float w3 = plot(uv + AvgAmp * 1.6, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 1.6)));
	float w4 = plot(uv + AvgAmp * 1.3, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 1.3)));
	float w5 = plot(uv + AvgAmp, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp)));
	float w6 = plot(uv + AvgAmp * 0.7, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 0.7)));
	float w7 = plot(uv + AvgAmp * 0.4, (blinnWyvillApx(uv.x * 0.5) * max(0.1, AvgAmp * 0.4)));
	return w1 + w2 + w3 + w4 + w5 + w6 + w7;
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
	uv.y = -uv.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
	mouse.y = -mouse.y;

	uv.x *= 1.4;

	uv.y = (uv.y < 0) ? uv.y : -uv.y;
	vec3 color = vec3(waves(uv) + plot(uv, 0.0)); 
	color *= palette(length(uv) * AvgAmp * 3.0, Color1, Color2, Color3, Color4);
	uv.x *= 0.6;
	color += length(uv) * 0.3 * AvgAmp * Color2;
	FragColor = vec4(color, 1.0);
}
