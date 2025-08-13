#version 330 core

// -~ Defines ~-
#define MARCH_STEPS 20
#define MIN_DISTANCE 0.01
#define MAX_DISTANCE 10.0

#define FOV 1.0

#define NORM_EPSILON 0.001

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

// -~ Structs ~-
struct Ray
{
        vec3 orig;
        vec3 dir;
};

// -~ Helpers ~-
float sphereSDF(in vec3 p, in float rad)
{
	return length(p) - rad;
}

float march(in vec3 p) 
{
	float s = sphereSDF(p, 0.5);
	return s;
}

vec3 calcNormal(in vec3 p)
{
	float d = march(p);
	vec2 delta = vec2(0.0, NORM_EPSILON);

	float xd = march(p + delta.yxx);
	float yd = march(p + delta.xyx);
	float zd = march(p + delta.xxy);

	return (normalize(vec3(xd, yd, zd) - d) + 1.0) * 0.5;
}

void main()
{
	// Normalizing screen coords
	vec2 uv = (gl_FragCoord.xy * 2.0 - Resolution) / Resolution.y;
	uv.y = -uv.y;

	// Normalizing mouse coords
	vec2 mouse = (Mouse * 2.0 - Resolution) / Resolution.y;
	mouse.y = -mouse.y;

	// Setting the ray for this pixel
	Ray ray;
        ray.orig = vec3(0.0, 0.0, -3.0);
        ray.dir = normalize(vec3(uv * FOV, 1.0));

        float t = 0.0; // total distance

        // Raymarching
        for (int i = 0; i < MARCH_STEPS; i++) {
                vec3 p = ray.orig + ray.dir * t;
                float d = march(p);
                t += d;
                if (d < MIN_DISTANCE || t > MAX_DISTANCE) break;
        }

	FragColor = vec4(vec3(t * 0.2), 1.0);
}
