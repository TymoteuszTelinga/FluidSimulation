#version 460 core

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;

layout(location = 0) out vec3 fragColor;

vec3 offsets[3] = 
{
	vec3(0.f, 0.f,  0.0f),
	vec3(0.f, 0.f,  0.5f),
	vec3(0.f, 0.f, -0.5f)
};

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(Position + offsets[gl_InstanceIndex], 1.0f);
    fragColor = Color;
}