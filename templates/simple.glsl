#version 330 core

// -~ Uniforms ~-
uniform uvec2 Resolution;
uniform vec2 Mouse;
uniform float Time;
uniform float PeakAmp;
uniform float AvgAmp;

// -~ Output ~-
out vec4 FragColor;

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
	mouse.y = -mouse.y;

	FragColor = vec4(1.0);
}
