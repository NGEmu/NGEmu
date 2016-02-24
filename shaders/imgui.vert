#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform PushConstants
{
  mat4 projection_matrix;
  int restrict_texture_samples;
} push_constants;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;
//layout(location = 2) in vec3 in_color;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec4 vtx_color;

void main() 
{
	// push_constants.projection_matrix
	gl_Position = vec4(in_pos.xyz, 1.0);
	//gl_Position.y = -gl_Position.y;
	out_color = in_color;
	//vtx_color = in_color;
}