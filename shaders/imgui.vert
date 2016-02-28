#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (push_constant) uniform UBO {
  mat4 projection_matrix;
} ubo;

layout(location = 0) in vec2 in_pos;
layout(location = 1) in vec2 in_UV;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec2 out_UV;
layout(location = 1) out vec4 out_color;

void main() 
{
	out_UV = in_UV;
	out_color = in_color;
	gl_Position = ubo.projection_matrix * vec4(in_pos.xy, 0, 1);
	gl_Position.y = -gl_Position.y;
}