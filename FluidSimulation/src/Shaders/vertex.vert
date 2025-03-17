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

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(Position, 1.0f);
    fragColor = Color;
}