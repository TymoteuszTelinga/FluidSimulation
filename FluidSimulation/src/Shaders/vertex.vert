#version 460 core

layout(set = 0, binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec3 Center; //per instance
layout(location = 4) in vec3 velocity; //per instance

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 UVcord;

void main()
{
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(Position + Center, 1.0f);
    fragColor = velocity;
	UVcord = UV;
}