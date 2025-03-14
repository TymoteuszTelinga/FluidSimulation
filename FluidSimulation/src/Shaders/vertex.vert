#version 460 core

vec2 positions[3] =
{
	vec2( 0.0, -0.5),
	vec2( 0.5,  0.5),
	vec2(-0.5,  0.5)
};

vec3 colors[3] =
{
    vec3(1.f, 0.f, 0.f),
    vec3(0.f, 1.f, 0.f),
    vec3(0.f, 0.f, 1.f)
};

layout(location = 0) out vec3 fragColor;

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
    fragColor = colors[gl_VertexIndex];
}