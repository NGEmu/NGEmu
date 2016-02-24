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
	VkVertexInputAttributeDescription vertex_input_attribute[3];
	VkVertexInputBindingDescription vertex_input_binding;
	VkRenderPass render_pass;

	// For font
	VkImage font_image;
	VkImageView font_image_view;
	VkSampler font_sampler;
	VkDeviceMemory device_memory;

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
	bool prepare_vulkan(void* handle, void* h_instance, u8 device_num);

	// ImGui
	static void imgui_render(ImDrawData* draw_data);
	u32 width;
	u32 height;
};