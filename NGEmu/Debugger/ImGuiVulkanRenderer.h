#pragma once

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define deg_to_rad(deg) deg * float(3.14159265358979323846 / 180)

struct Texture
{
	VkSampler sampler;
	VkImage image;
	VkImageView image_view;
	VkImageLayout image_layout;
	VkDeviceMemory device_memory;
	u32 width, height, bytes_per_pixel;
	u8* pixels;
};

struct Vertex
{
	float pos[3];
	float col[3];
};

class ImGuiVulkanRenderer
{
public:
	ImGuiVulkanRenderer::~ImGuiVulkanRenderer();
	bool initialize(void* handle, void* h_instance, u8 device_num = 0);
	void new_frame(u32 width, u32 height);

	// Vulkan (DO NOT ACCESS THESE)
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
	VkPhysicalDeviceMemoryProperties memory_properties;

	// Rendering (DO NOT ACCESS THESE EITHER)
	VkImage swapchain_images[2];
	VkImageView swapchain_image_views[2];
	VkPipeline pipeline;
	VkPipelineCache pipeline_cache;
	VkPipelineLayout pipeline_layout;
	VkDescriptorSet descriptor_set;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkVertexInputAttributeDescription vertex_input_attribute[2];
	VkVertexInputBindingDescription vertex_input_binding;
	VkBuffer vertex_buffer;
	VkBuffer index_buffer;
	VkRenderPass render_pass;
	Texture font_texture;

	// Triangle
	struct
	{
		glm::mat4 projection_matrix;
		glm::mat4 model_matrix;
		glm::mat4 view_matrix;
	} uniform_buffer;

	struct
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	}  uniform_data;

	struct
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
	} triangle_vertices;

	struct
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
	} triangle_indices;

	// For convenience
	VkBool32 get_memory_type(u32 typeBits, VkFlags properties, u32 *typeIndex);

private:
	// For debug output
	PFN_vkCreateDebugReportCallbackEXT create_debug_report;
	PFN_vkDestroyDebugReportCallbackEXT destroy_debug_report;
	VkDebugReportCallbackEXT debug_callback_function;
	static VkBool32 debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, u64 src, u64 location, s32 msg_code, char* prefix, char* msg, void* user_data);

	// For convenience
	u32 get_graphics_family(VkPhysicalDevice adapter, VkSurfaceKHR window_surface);
	VkShaderModule load_shader_GLSL(std::string file_name);

	// Internal functions for Vulkan
	bool create_vulkan_basics(void* handle, void* h_instance, u8 device_num);
	bool prepare_fonts();

	// ImGui
	static void imgui_render(ImDrawData* draw_data);
	u32 width;
	u32 height;
};