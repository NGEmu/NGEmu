#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_UV;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main() 
{
	out_color = in_color * texture(texture_sampler, in_UV);
}