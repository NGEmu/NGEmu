#include "stdafx.h"
#include "ImGuiVulkanRenderer.h"

// NOTE: This code is terrible and makes you puke. I know it. Here's a picture of Vulkan API, as described by Nvidia's driver team:
//                                        ,   ,  
//                                        $,  $,     ,            
//                                        "ss.$ss. .s'     
//                                ,     .ss$$$$$$$$$$s,              
//                                $. s$$$$$$$$$$$$$$`$$Ss       
//                                "$$$$$$$$$$$$$$$$$$o$$$       ,       
//                               s$$$$$$$$$$$$$$$$$$$$$$$$s,  ,s  
//                              s$$$$$$$$$"$$$$$$""""$$$$$$"$$$$$,     
//                              s$$$$$$$$$$s""$$$$ssssss"$$$$$$$$"   
//                             s$$$$$$$$$$'         `"""ss"$"$s""      
//                             s$$$$$$$$$$,              `"""""$  .s$$s
//                             s$$$$$$$$$$$$s,...               `s$$'  `
//                         `ssss$$$$$$$$$$$$$$$$$$$$####s.     .$$"$.   , s-
//                           `""""$$$$$$$$$$$$$$$$$$$$#####$$$$$$"     $.$'
//                                 "$$$$$$$$$$$$$$$$$$$$$####s""     .$$$|
//                                  "$$$$$$$$$$$$$$$$$$$$$$$$##s    .$$" $ 
//                                   $$""$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"   `
//                                  $$"  "$"$$$$$$$$$$$$$$$$$$$$S""""' 
//                             ,   ,"     '  $$$$$$$$$$$$$$$$####s  
//                             $.          .s$$$$$$$$$$$$$$$$$####"
//                 ,           "$s.   ..ssS$$$$$$$$$$$$$$$$$$$####"
//                 $           .$$$S$$$$$$$$$$$$$$$$$$$$$$$$#####"
//                 Ss     ..sS$$$$$$$$$$$$$$$$$$$$$$$$$$$######""
//                  "$$sS$$$$$$$$$$$$$$$$$$$$$$$$$$$########"
//           ,      s$$$$$$$$$$$$$$$$$$$$$$$$#########""'
//           $    s$$$$$$$$$$$$$$$$$$$$$#######""'      s'         ,
//           $$..$$$$$$$$$$$$$$$$$$######"'       ....,$$....    ,$
//            "$$$$$$$$$$$$$$$######"' ,     .sS$$$$$$$$$$$$$$$$s$$
//              $$$$$$$$$$$$#####"     $, .s$$$$$$$$$$$$$$$$$$$$$$$$s.
//   )          $$$$$$$$$$$#####'      `$$$$$$$$$###########$$$$$$$$$$$.
//  ((          $$$$$$$$$$$#####       $$$$$$$$###"       "####$$$$$$$$$$ 
//  ) \         $$$$$$$$$$$$####.     $$$$$$###"             "###$$$$$$$$$   s'
// (   )        $$$$$$$$$$$$$####.   $$$$$###"                ####$$$$$$$$s$$'
// )  ( (       $$"$$$$$$$$$$$#####.$$$$$###' -Tua Xiong     .###$$$$$$$$$$"
// (  )  )   _,$"   $$$$$$$$$$$$######.$$##'                .###$$$$$$$$$$
// ) (  ( \.         "$$$$$$$$$$$$$#######,,,.          ..####$$$$$$$$$$$"
//(   )$ )  )        ,$$$$$$$$$$$$$$$$$$####################$$$$$$$$$$$"        
//(   ($$  ( \     _sS"  `"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$S$$,       
// )  )$$$s ) )  .      .   `$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"'  `$$   
//  (   $$$Ss/  .$,    .$,,s$$$$$$##S$$$$$$$$$$$$$$$$$$$$$$$$S""        ' 
//    \)_$$$$$$$$$$$$$$$$$$$$$$$##"  $$        `$$.        `$$.
//        `"S$$$$$$$$$$$$$$$$$#"      $          `$          `$
//            `"""""""""""""'         '           '           '

// Get the first device that supports graphics output and surfaces
u32 ImGuiVulkanRenderer::get_graphics_family(VkPhysicalDevice adapter, VkSurfaceKHR window_surface)
{
	// Get the amount of family queues
	u32 family_queue_count;
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &family_queue_count, nullptr);

	// Get list of family queues
	std::vector<VkQueueFamilyProperties> queues(family_queue_count);
	vkGetPhysicalDeviceQueueFamilyProperties(adapter, &family_queue_count, queues.data());

	for (u32 i = 0; i < queues.size(); ++i)
	{
		if (!(queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			continue;
		}

		VkBool32 supported;

		if (vkGetPhysicalDeviceSurfaceSupportKHR(adapter, i, window_surface, &supported) != VK_SUCCESS)
		{
			log(ERROR, "Failed to query surface support.");
			return 0xBAD;
		}

		if (supported)
		{
			return i;
		}
	}

	return 0xBAD;
}

VkBool32 ImGuiVulkanRenderer::get_memory_type(u32 typeBits, VkFlags properties, u32 *typeIndex)
{
	for (u8 i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				*typeIndex = i;
				return true;
			}
		}

		typeBits >>= 1;
	}

	return false;
}

VkShaderModule ImGuiVulkanRenderer::load_shader_GLSL(std::string file_name)
{
	std::ifstream stream(file_name, std::ios::binary);

	if (!stream)
	{
		log(ERROR, "Failed to open shader file.");
		return nullptr;
	}

	stream.seekg(0, std::ios::end);
	u64 size = stream.tellg();
	stream.seekg(0, std::ios::beg);

	char* shader_code = (char*)malloc(size);
	stream.read(shader_code, size);

	VkShaderModuleCreateInfo shader_module_info = {};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize = size;
	shader_module_info.pCode = (u32*)shader_code;

	VkShaderModule shader_module;
	VkResult result;

	if ((result = vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a shader module. (%d)", result);
		return nullptr;
	}

	// TODO: Free, if it's freeable. Gotta be sure.
	//free(shader_code);

	return shader_module;
}

VkBool32 ImGuiVulkanRenderer::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, u64 src, u64 location, s32 msg_code, char* prefix, char* msg, void* user_data)
{
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		log(ERROR, "[%s]: %s", prefix, msg);
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT || flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		log(WARNING, "[%s]: %s", prefix, msg);
	}
	else
	{
		log(WARNING, "[%s]: %s", prefix, msg);
	}

	return false;
}

ImGuiVulkanRenderer::~ImGuiVulkanRenderer()
{
	// We need to check if these objects exist, or else we'll crash
	if (destroy_debug_report)
	{
		destroy_debug_report(instance, debug_callback_function, nullptr);
	}

	if (font_texture.image_view)
	{
		vkDestroyImageView(device, font_texture.image_view, nullptr);
	}

	if (font_texture.image)
	{
		vkDestroyImage(device, font_texture.image, nullptr);
	}

	if (font_texture.device_memory)
	{
		vkFreeMemory(device, font_texture.device_memory, nullptr);
	}

	if (command_pool)
	{
		vkDestroyCommandPool(device, command_pool, nullptr);
	}

	if (swapchain)
	{
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	}

	if (surface)
	{
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

	if (device)
	{
		vkDestroyDevice(device, nullptr);
	}

	if (instance)
	{
		vkDestroyInstance(instance, nullptr);
	}
}

void ImGuiVulkanRenderer::new_frame(u32 width_in, u32 height_in)
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)width_in, (float)height_in);
	width = width_in;
	height = height_in;

	ImGui::NewFrame();
}

void ImGuiVulkanRenderer::imgui_render(ImDrawData* draw_data)
{
	ImGuiVulkanRenderer& renderer = *(ImGuiVulkanRenderer*)ImGui::GetIO().UserData;
	ImGuiIO& io = ImGui::GetIO();

	VkResult result;
	u32 current_buffer;

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphore semaphore;

	if ((result = vkCreateSemaphore(renderer.device, &semaphore_info, nullptr, &semaphore)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a semaphore. (%d)", result);
		return;
	}

	if ((result = vkAcquireNextImageKHR(renderer.device, renderer.swapchain, UINT64_MAX, semaphore, nullptr, &current_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the index of the next image in the chain. (%d)", result);
		return;
	}

	VkCommandBufferBeginInfo command_buffer_begin = {};
	command_buffer_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((result = vkBeginCommandBuffer(renderer.command_buffer, &command_buffer_begin)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to begin the command buffer. (%d)", result);
		return;
	}

	VkFramebufferCreateInfo framebuffer_info = {};
	framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_info.width = renderer.width;
	framebuffer_info.height = renderer.height;
	framebuffer_info.renderPass = renderer.render_pass;
	framebuffer_info.attachmentCount = 1;
	framebuffer_info.layers = 1;
	framebuffer_info.pAttachments = &renderer.swapchain_image_views[current_buffer];

	VkFramebuffer framebuffer;

	if ((result = vkCreateFramebuffer(renderer.device, &framebuffer_info, nullptr, &framebuffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a framebuffer. (%d)", result);
		return;
	}

	VkClearValue clear_value[2];
	clear_value[0].color.float32[0] = 0.025f;
	clear_value[0].color.float32[1] = 0.025f;
	clear_value[0].color.float32[2] = 0.025f;
	clear_value[0].depthStencil.stencil = 1;
	clear_value[1].depthStencil.depth = 1.0f;
	clear_value[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.framebuffer = framebuffer;
	render_pass_begin_info.renderPass = renderer.render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = renderer.width;
	render_pass_begin_info.renderArea.extent.height = renderer.height;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_value;

	vkCmdBeginRenderPass(renderer.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.width = (float)renderer.width;
	viewport.height = (float)renderer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(renderer.command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.extent.width = 1100;
	scissor.extent.height = 800;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(renderer.command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline_layout, 0, 1, &renderer.descriptor_set, 0, nullptr);
	vkCmdBindPipeline(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);

	// Bind triangle vertices
	u64 offsets = 0;
	vkCmdBindVertexBuffers(renderer.command_buffer, 0, 1, &renderer.triangle_vertices.buffer, &offsets);

	// Bind triangle indices
	vkCmdBindIndexBuffer(renderer.command_buffer, renderer.triangle_indices.buffer, 0, VK_INDEX_TYPE_UINT32);

	// Draw the indexed triangle
	vkCmdDrawIndexed(renderer.command_buffer, 3, 1, 0, 0, 1);

	if (draw_data->CmdListsCount != 1)
	{
		log(DEBUG, "%d", draw_data->CmdListsCount);
	}

	//for (s32 i = 0; i < draw_data->CmdListsCount; i++)
	//{
	//	ImDrawList* draw_list = draw_data->CmdLists[i];
	//	//u8* vtx_buffer = (u8*)&draw_list->VtxBuffer.front();
	//	//u8* idx_buffer = (u8*)&draw_list->IdxBuffer.front();
	//	
	//	if (renderer.vertex_buffer && renderer.index_buffer)
	//	{
	//		vkDestroyBuffer(renderer.device, renderer.vertex_buffer, nullptr);
	//		vkDestroyBuffer(renderer.device, renderer.index_buffer, nullptr);
	//	}

	//	// Create the index and vertex buffers
	//	// Vertex buffer info
	//	VkBufferCreateInfo vertex_buffer_info = {};
	//	vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//	vertex_buffer_info.size = draw_list->VtxBuffer.size();
	//	vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	//	// Index buffer info
	//	VkBufferCreateInfo index_buffer_info = {};
	//	index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//	index_buffer_info.size = draw_list->IdxBuffer.size();
	//	index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	//	if ((result = vkCreateBuffer(renderer.device, &vertex_buffer_info, nullptr, &renderer.vertex_buffer)) != VK_SUCCESS)
	//	{
	//		log(ERROR, "Failed to create an index buffer for rendering. (%d)", result);
	//		return;
	//	}

	//	if ((result = vkCreateBuffer(renderer.device, &index_buffer_info, nullptr, &renderer.index_buffer)) != VK_SUCCESS)
	//	{
	//		log(ERROR, "Failed to create an index buffer for rendering. (%d)", result);
	//		return;
	//	}

	//	void *data;
	//	VkDeviceMemory device_memory;
	//	VkMemoryRequirements memory_requirements;
	//	VkMemoryAllocateInfo memory_allocate_info = {};
	//	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	//	vkGetBufferMemoryRequirements(renderer.device, renderer.index_buffer, &memory_requirements);

	//	memory_allocate_info.allocationSize = memory_requirements.size;

	//	if (!renderer.get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex))
	//	{
	//		log(ERROR, "Failed to get memory type for index buffer. (%d)", result);
	//		return;
	//	}

	//	if ((result = vkAllocateMemory(renderer.device, &memory_allocate_info, nullptr, &device_memory)) != VK_SUCCESS)
	//	{
	//		log(ERROR, "Failed to allocate memory for the index buffer. (%d)", result);
	//		return;
	//	}

	//	if ((result = vkMapMemory(renderer.device, device_memory, 0, memory_allocate_info.allocationSize, 0, &data)) != VK_SUCCESS)
	//	{
	//		log(ERROR, "Failed to map memory for the index buffer. (%d)", result);
	//		return;
	//	}

	//	memcpy(data, draw_list->IdxBuffer.begin(), draw_list->IdxBuffer.size());

	//	vkUnmapMemory(renderer.device, device_memory);

	//	if ((result = vkBindBufferMemory(renderer.device, renderer.index_buffer, device_memory, 0)) != VK_SUCCESS)
	//	{
	//		log(ERROR, "Failed to bind memory to the index buffer. (%d)", result);
	//		return;
	//	}

	//	//vkCmdBindVertexBuffers(renderer.command_buffer, );
	//	vkCmdBindIndexBuffer(renderer.command_buffer, renderer.index_buffer, 0, VK_INDEX_TYPE_UINT32);

	//	for (s32 j = 0; j < draw_list->CmdBuffer.size(); j++)
	//	{
	//		ImDrawCmd* draw_cmd = &draw_list->CmdBuffer[j];

	//		if (draw_cmd->UserCallback)
	//		{
	//			draw_cmd->UserCallback(draw_list, draw_cmd);
	//		}
	//		else
	//		{
	//			VkRect2D scissor;
	//			//scissor.extent.width = (u32)(draw_cmd->ClipRect.z - draw_cmd->ClipRect.x);
	//			vkCmdSetScissor(renderer.command_buffer, 0, 1, &scissor);

	//			vkCmdDrawIndexed(renderer.command_buffer, draw_cmd->ElemCount, 1, 0, 0, 0);
	//		}
	//		//idx_buffer += draw_cmd->ElemCount;
	//	}

	//	vkFreeMemory(renderer.device, device_memory, nullptr);
	//}

	vkCmdEndRenderPass(renderer.command_buffer);

	if ((result = vkEndCommandBuffer(renderer.command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to end the command buffer. (%d)", result);
		return;
	}

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &semaphore;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &renderer.command_buffer;

	if ((result = vkQueueSubmit(renderer.queue, 1, &submit_info, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to submit to the queue. (%d)", result);
		return;
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &renderer.swapchain;
	present_info.pImageIndices = &current_buffer;

	if ((result = vkQueuePresentKHR(renderer.queue, &present_info)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to present swapchain image. (%d)", result);
		return;
	}

	vkDestroySemaphore(renderer.device, semaphore, nullptr);

	if ((result = vkQueueWaitIdle(renderer.queue)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to wait for the queue to become idle. (%d)", result);
		return;
	}
}

bool ImGuiVulkanRenderer::create_vulkan_basics(void* handle, void* h_instance, u8 device_num)
{
	// The names of validation layers
	const char* validation_layer_names[] =
	{
		"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_GOOGLE_unique_objects",
		"VK_LAYER_LUNARG_api_dump",
	};

	std::vector<const char*> instance_extensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	std::vector<const char*> layer_names;

	// Add all the validation layers (change to 3, to enable API call dumping)
	for (u8 i = 0; i < 2; i++)
	{
		layer_names.push_back(validation_layer_names[i]);
	}

	// Set application info
	VkApplicationInfo application_info = {};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "NGEmu";
	application_info.pEngineName = "NGEmu";
	application_info.apiVersion = VK_MAKE_VERSION(1, 0, 3);

	// Set Vulkan instance info and create the instance
	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &application_info;
	instance_info.enabledLayerCount = (u32)layer_names.size();
	instance_info.ppEnabledLayerNames = layer_names.data();
	instance_info.enabledExtensionCount = (u32)instance_extensions.size();
	instance_info.ppEnabledExtensionNames = instance_extensions.data();

	VkResult result;

	if ((result = vkCreateInstance(&instance_info, nullptr, &instance)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a Vulkan instance. (%d)", result);
		return false;
	}

	// Set up the debug callback
	create_debug_report = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	destroy_debug_report = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");

	if (!create_debug_report || !destroy_debug_report)
	{
		log(ERROR, "Failed to obtain debug reporting function addresses. (%d)", result);
		return false;
	}

	VkDebugReportFlagsEXT flags = {};
	flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
	flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
	flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

	VkDebugReportCallbackCreateInfoEXT debug_callback_info = {};
	debug_callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_callback_info.pfnCallback = (PFN_vkDebugReportCallbackEXT)debug_callback;
	debug_callback_info.flags = flags;

	create_debug_report(instance, &debug_callback_info, nullptr, &debug_callback_function);

	// Find an appropriate device
	u32 device_count;

	if ((result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of devices. (%d)", result);
		return false;
	}

	std::vector<VkPhysicalDevice> adapters(device_count);

	if ((result = vkEnumeratePhysicalDevices(instance, &device_count, adapters.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain devices. (%d)", result);
		return false;
	}

	physical_device = adapters[device_num];

	// Create a win32 window surface
	VkWin32SurfaceCreateInfoKHR window_surface_info = {};
	window_surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	window_surface_info.hwnd = (HWND)handle;
	window_surface_info.hinstance = (HINSTANCE)h_instance;

	if ((result = vkCreateWin32SurfaceKHR(instance, &window_surface_info, nullptr, &surface)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create window surface. (%d)", result);
		return false;
	}

	// Get the first graphic family, that supports Vulkan
	u32 queue_family = get_graphics_family(physical_device, surface);

	if (queue_family == 0xBAD)
	{
		log(ERROR, "Failed to obtain a graphics family, that supports Vulkan. (%d)", result);
		return false;
	}

	// Create a Vulkan device
	float queue_priority = 1;

	VkDeviceQueueCreateInfo device_queue_info = {};
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_info.queueFamilyIndex = queue_family;
	device_queue_info.queueCount = 1;
	device_queue_info.pQueuePriorities = &queue_priority;

	std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.enabledLayerCount = (u32)layer_names.size();
	device_info.ppEnabledLayerNames = layer_names.data();
	device_info.enabledExtensionCount = (u32)device_extensions.size();
	device_info.ppEnabledExtensionNames = device_extensions.data();

	if ((result = vkCreateDevice(physical_device, &device_info, nullptr, &device)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a Vulkan device handle. (%d)", result);
		return false;
	}

	// Get the queue
	vkGetDeviceQueue(device, queue_family, 0, &queue);

	// Get the memory properties
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	// Get surface capabilities, formats and presentation modes
	u32 format_count;
	u32 present_mode_count;

	if ((vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of device surface formats. (%d)", result);
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of device surface present modes. (%d)", result);
		return false;
	}

	std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
	std::vector<VkPresentModeKHR> present_modes(present_mode_count);

	if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the surface formats. (%d)", result);
		return false;
	}

	if ((result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data())) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the surface present modes. (%d)", result);
		return false;
	}

	VkSurfaceCapabilitiesKHR surface_capabilities;

	if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of device surface present modes. (%d)", result);
		return false;
	}

	// Create a swapchain
	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageExtent = surface_capabilities.currentExtent;
	swapchain_info.imageFormat = surface_formats[0].format;
	swapchain_info.presentMode = present_modes[0];
	swapchain_info.minImageCount = 2;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	if ((result = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a swapchain. (%d)", result);
		return false;
	}

	u32 swapchain_image_count;

	if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
		return false;
	}

	if ((result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain the number of swapchain images. (%d)", result);
		return false;
	}

	// Create a command pool
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.queueFamilyIndex = get_graphics_family(physical_device, surface);;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if ((result = vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a command pool. (%d)", result);
		return false;
	}

	// Allocate a command buffer
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = 1;

	if ((result = vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate a command buffer. (%d)", result);
		return false;
	}

	// Begin a command buffer
	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to begin a command buffer. (%d)", result);
		return false;
	}

	// Create image views for the swapchain images
	for (u8 i = 0; i < 2; ++i)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		barrier.image = swapchain_images[i];

		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageViewCreateInfo m_swap_chain_image_view = {};
		m_swap_chain_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		m_swap_chain_image_view.format = surface_formats[0].format;
		m_swap_chain_image_view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		m_swap_chain_image_view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		m_swap_chain_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		m_swap_chain_image_view.image = swapchain_images[i];

		if ((result = vkCreateImageView(device, &m_swap_chain_image_view, nullptr, &swapchain_image_views[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to create swapchain image view. (%d)", result);
			return false;
		}
	}

	if ((result = vkEndCommandBuffer(command_buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to end the command buffer. (%d)", result);
		return false;
	}

	// Create a render pass
	VkAttachmentDescription attachement_description = {};
	attachement_description.format = VK_FORMAT_R8G8B8A8_UNORM;
	attachement_description.samples = VK_SAMPLE_COUNT_1_BIT;
	attachement_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachement_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachement_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachement_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachement_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachement_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference attachment_reference = {};
	attachment_reference.attachment = 0;
	attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description = {};
	subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &attachment_reference;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass_description;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &attachement_description;

	if ((result = vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a render pass. (%d)", result);
		return false;
	}

	// Create a descriptor set layout
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
	descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &descriptor_set_layout_binding;

	if ((result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a descriptor set layout. (%d)", result);
		return false;
	}

	// Create the pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;

	if ((result = vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a pipeline layout. (%d)", result);
		return false;
	}

	// Prepare vertex input attribute
	vertex_input_attribute[0].location = 0;
	vertex_input_attribute[0].format = VK_FORMAT_R32G32B32_SFLOAT;

	vertex_input_attribute[1].location = 1;
	vertex_input_attribute[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute[1].offset = sizeof(float) * 3;

	// Prepare vertex binding attributes
	vertex_input_binding.binding = 0;
	vertex_input_binding.stride = sizeof(Vertex);
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Create a rendering pipeline
	// Vertex input info
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexAttributeDescriptionCount = 2;
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding;

	// Vertex input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport info
	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.scissorCount = 1;

	// Rasterization info
	VkPipelineRasterizationStateCreateInfo rasterization_info = {};
	rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_info.cullMode = VK_CULL_MODE_NONE;
	rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_info.depthClampEnable = VK_FALSE;
	rasterization_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_info.depthBiasEnable = VK_FALSE;

	// Multisampling info
	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth stencil info
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable = VK_TRUE;
	depth_stencil_info.depthWriteEnable = VK_TRUE;
	depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_info.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil_info.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil_info.stencilTestEnable = VK_FALSE;
	depth_stencil_info.front = depth_stencil_info.back;

	// Color blending info
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = 0xF;
	color_blend_attachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.attachmentCount = 1;
	color_blend_info.pAttachments = &color_blend_attachment;

	// Dynamic states
	std::vector<VkDynamicState> dynamic_states =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_info;
	dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_info.dynamicStateCount = (u32)dynamic_states.size();
	dynamic_info.pDynamicStates = dynamic_states.data();

	// Vertex shaders
	VkPipelineShaderStageCreateInfo shader_info[2] = {};
	shader_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_info[0].module = load_shader_GLSL("../shaders/imgui.vert.spv");
	shader_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_info[0].pName = "main"; // Setting this to anything else crashes the driver.

	shader_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_info[1].module = load_shader_GLSL("../shaders/imgui.frag.spv");
	shader_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_info[1].pName = "main"; // Setting this to anything else crashes the driver.
	
	// Create the pipeline cache
	VkPipelineCacheCreateInfo pipeline_cache_info = {};
	pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	if ((result = vkCreatePipelineCache(device, &pipeline_cache_info, nullptr, &pipeline_cache)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a pipeline cache. (%d)", result);
		return false;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_info;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState = &viewport_info;
	pipeline_info.pRasterizationState = &rasterization_info;
	pipeline_info.pMultisampleState = &multisample_info;
	pipeline_info.pDepthStencilState = &depth_stencil_info;
	pipeline_info.pColorBlendState = &color_blend_info;
	pipeline_info.pDynamicState = &dynamic_info;
	pipeline_info.renderPass = render_pass;
	pipeline_info.layout = pipeline_layout;

	if ((result = vkCreateGraphicsPipelines(device, pipeline_cache, 1, &pipeline_info, nullptr, &pipeline)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a graphics pipeline. (%d)", result);
		return false;
	}

	// Setup a descriptor pool
	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_info = {};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.poolSizeCount = 1;
	descriptor_pool_info.pPoolSizes = &descriptor_pool_size;
	descriptor_pool_info.maxSets = 1;

	if ((result = vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr, &descriptor_pool)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a descriptor pool. (%d)", result);
		return false;
	}

	// Setup up the descriptor set
	VkDescriptorSetAllocateInfo descriptor_set_info = {};
	descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_info.descriptorPool = descriptor_pool;
	descriptor_set_info.descriptorSetCount = 1;
	descriptor_set_info.pSetLayouts = &descriptor_set_layout;

	if ((result = vkAllocateDescriptorSets(device, &descriptor_set_info, &descriptor_set)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate a descriptor set. (%d)", result);
		return false;
	}

	// Create a triangle buffer
	std::vector<Vertex> vertex_buffer =
	{
		{{ 1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
		{{ -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f }},
		{{ 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
	};

	std::vector<u32> index_buffer = { 0, 1, 2 };

	// Vertex buffer info
	VkBufferCreateInfo vertex_buffer_info = {};
	vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertex_buffer_info.size = vertex_buffer.size() * sizeof(Vertex);
	vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	// Index buffer info
	VkBufferCreateInfo index_buffer_info = {};
	index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	index_buffer_info.size = index_buffer.size() * sizeof(u32);
	index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	if ((result = vkCreateBuffer(device, &vertex_buffer_info, nullptr, &triangle_vertices.buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create an index buffer for rendering. (%d)", result);
		return false;
	}

	if ((result = vkCreateBuffer(device, &index_buffer_info, nullptr, &triangle_indices.buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create an index buffer for rendering. (%d)", result);
		return false;
	}

	void *data;
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	// Triangle vertices
	vkGetBufferMemoryRequirements(device, triangle_vertices.buffer, &memory_requirements);

	memory_allocate_info.allocationSize = memory_requirements.size;

	if (!get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get memory type for index buffer. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocate_info, nullptr, &triangle_vertices.memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate memory for the index buffer. (%d)", result);
		return false;
	}

	if ((result = vkMapMemory(device, triangle_vertices.memory, 0, memory_allocate_info.allocationSize, 0, &data)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for the index buffer. (%d)", result);
		return false;
	}

	memcpy(data, vertex_buffer.data(), vertex_buffer.size() * sizeof(Vertex));

	vkUnmapMemory(device, triangle_vertices.memory);

	if ((result = vkBindBufferMemory(device, triangle_vertices.buffer, triangle_vertices.memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory to the index buffer. (%d)", result);
		return false;
	}

	// Triangle indices
	vkGetBufferMemoryRequirements(device, triangle_indices.buffer, &memory_requirements);

	memory_allocate_info.allocationSize = memory_requirements.size;

	if (!get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get memory type for index buffer. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocate_info, nullptr, &triangle_indices.memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate memory for the index buffer. (%d)", result);
		return false;
	}

	if ((result = vkMapMemory(device, triangle_indices.memory, 0, memory_allocate_info.allocationSize, 0, &data)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for the index buffer. (%d)", result);
		return false;
	}

	memcpy(data, index_buffer.data(), index_buffer.size() * sizeof(u32));

	vkUnmapMemory(device, triangle_indices.memory);

	if ((result = vkBindBufferMemory(device, triangle_indices.buffer, triangle_indices.memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory to the index buffer. (%d)", result);
		return false;
	}

	// Prepare the uniform stuff
	VkMemoryRequirements memReqs;
	
	// Vertex shader uniform buffer block
	VkBufferCreateInfo uniform_buffer_info = {};
	uniform_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	uniform_buffer_info.size = sizeof(uniform_buffer);
	uniform_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	if ((result = vkCreateBuffer(device, &uniform_buffer_info, nullptr, &uniform_data.buffer)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create uniform buffer. (%d)", result);
		return false;
	}

	vkGetBufferMemoryRequirements(device, uniform_data.buffer, &memReqs);

	memory_allocate_info.allocationSize = memReqs.size;

	if (!get_memory_type(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get memory type for uniform buffer. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocate_info, nullptr, &uniform_data.memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate memory for uniform buffer. (%d)", result);
		return false;
	}

	if ((result = vkBindBufferMemory(device, uniform_data.buffer, uniform_data.memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory for uniform buffer. (%d)", result);
		return false;
	}

	uniform_data.descriptor.buffer = uniform_data.buffer;
	uniform_data.descriptor.offset = 0;
	uniform_data.descriptor.range = sizeof(uniform_buffer);

	// Uniform buffer stuff
	glm::vec3 rotation = glm::vec3();
	rotation.x = 1;
	rotation.y = 1;
	rotation.z = 1;

	uniform_buffer.projection_matrix = glm::perspective(deg_to_rad(60.0f), (float)1100 / (float)800, 0.1f, 256.0f);

	uniform_buffer.view_matrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -2.5f));

	uniform_buffer.model_matrix = glm::mat4();
	uniform_buffer.model_matrix = glm::rotate(uniform_buffer.model_matrix, deg_to_rad(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	uniform_buffer.model_matrix = glm::rotate(uniform_buffer.model_matrix, deg_to_rad(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	uniform_buffer.model_matrix = glm::rotate(uniform_buffer.model_matrix, deg_to_rad(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Map uniform buffer and update it
	u8* data2;

	if ((result = vkMapMemory(device, uniform_data.memory, 0, sizeof(uniform_buffer), 0, (void **)&data2)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for uniform buffer. (%d)", result);
		return false;
	}

	memcpy(data2, &uniform_buffer, sizeof(uniform_buffer));
	vkUnmapMemory(device, uniform_data.memory);

	// Update descriptor sets
	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstBinding = 0;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_set.pBufferInfo = &uniform_data.descriptor;

	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

	return true;
}

bool ImGuiVulkanRenderer::prepare_fonts()
{
	ImGuiIO& io = ImGui::GetIO();
	VkResult result;

	// Upload the font texture to the GPU and get the font data from ImGui
	io.Fonts->GetTexDataAsRGBA32(&font_texture.pixels, (s32*)&font_texture.width, (s32*)&font_texture.height, (s32*)&font_texture.bytes_per_pixel);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.flags = 0;
	image_info.extent.width = font_texture.width;
	image_info.extent.height = font_texture.height;
	image_info.extent.depth = 1;

	if ((result = vkCreateImage(device, &image_info, nullptr, &font_texture.image)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a font image. (%d)", result);
		return false;
	}

	// Get the memory requirements and allocate memory on the GPU
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, font_texture.image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;

	if (!get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get memory type for font texture. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocate_info, nullptr, &font_texture.device_memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for the font texture. (%d)", result);
		return false;
	}

	// Bind the allocated memory to the texture
	if ((result = vkBindImageMemory(device, font_texture.image, font_texture.device_memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory to the font texture. (%d)", result);
		return false;
	}

	// Image data pointer
	void *image_data;

	// Get image sub resource layout
	VkImageSubresource image_subresource = {};
	image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkSubresourceLayout subresource_layout = {};

	vkGetImageSubresourceLayout(device, font_texture.image, &image_subresource, &subresource_layout);

	if ((result = vkMapMemory(device, font_texture.device_memory, 0, memory_requirements.size, 0, &image_data)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to obtain image subresource layout. (%d)", result);
		return false;
	}

	// Copy the texture to memory
	memcpy(image_data, font_texture.pixels, font_texture.width * font_texture.height * font_texture.bytes_per_pixel);
	vkUnmapMemory(device, font_texture.device_memory);

	font_texture.image_layout = VK_IMAGE_LAYOUT_GENERAL;

	// Create a sampler for the image
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	if ((result = vkCreateSampler(device, &sampler_info, nullptr, &font_texture.sampler)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create the font image sampler. (%d)", result);
		return false;
	}

	// Create an image view
	VkImageViewCreateInfo image_view_info = {};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_info.subresourceRange.layerCount = 1;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.image = font_texture.image;

	if ((result = vkCreateImageView(device, &image_view_info, nullptr, &font_texture.image_view)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create an image view. (%d)", result);
		return false;
	}

	io.Fonts->TexID = (void*)&font_texture.image_view;

	return true;
}

bool ImGuiVulkanRenderer::initialize(void* handle, void* h_instance, u8 device_num)
{
	// Set some basic ImGui info
	ImGuiIO& io = ImGui::GetIO();
	io.ImeWindowHandle = handle;
	io.RenderDrawListsFn = imgui_render;
	io.UserData = this;

	if (!create_vulkan_basics(handle, h_instance, device_num))
	{
		log(ERROR, "Failed to initialize Vulkan renderer.");
		return false;
	}

	if (!prepare_fonts())
	{
		log(ERROR, "Failed to prepare fonts for use.");
		return false;
	}

	return true;
}