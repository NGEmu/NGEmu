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
// )  ( (       $$"$$$$$$$$$$$#####.$$$$$###'                .###$$$$$$$$$$"
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

	for (u32 i = 0; i < queues.size(); i++)
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

	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.025f;
	clear_value.color.float32[1] = 0.025f;
	clear_value.color.float32[2] = 0.025f;

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.framebuffer = framebuffer;
	render_pass_begin_info.renderPass = renderer.render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = renderer.width;
	render_pass_begin_info.renderArea.extent.height = renderer.height;
	render_pass_begin_info.clearValueCount = 1;
	render_pass_begin_info.pClearValues = &clear_value;

	vkCmdBeginRenderPass(renderer.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {};
	viewport.width = (float)renderer.width;
	viewport.height = (float)renderer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(renderer.command_buffer, 0, 1, &viewport);

	//bool restrict = false;
	vkCmdBindDescriptorSets(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline_layout, 0, 1, &renderer.descriptor_set, 0, nullptr);
	//vkCmdPushConstants(renderer.command_buffer, renderer.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(float) * 16, sizeof(s32), &restrict);
	vkCmdBindPipeline(renderer.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);

	if (draw_data->CmdListsCount != 1)
	{
		log(DEBUG, "Ah dang. Report this to tambre. (%d)", draw_data->CmdListsCount);
	}

	std::vector<VkDeviceMemory> buffer_memory(draw_data->CmdListsCount);
	std::vector<VkBuffer> vertex_buffer(draw_data->CmdListsCount);
	std::vector<VkBuffer> index_buffer(draw_data->CmdListsCount);

	for (s32 i = 0; i < draw_data->CmdListsCount; i++)
	{
		ImDrawList* draw_list = draw_data->CmdLists[i];

		VkBufferCreateInfo vertex_buffer_info = {};
		vertex_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vertex_buffer_info.size = draw_list->VtxBuffer.size() * sizeof(ImDrawVert);
		vertex_buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VkBufferCreateInfo index_buffer_info = {};
		index_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		index_buffer_info.size = draw_list->IdxBuffer.size() * sizeof(ImDrawIdx);
		index_buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		if ((result = vkCreateBuffer(renderer.device, &vertex_buffer_info, nullptr, &vertex_buffer[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to create a vertex buffer for rendering. (%d)", result);
			return;
		}

		if ((result = vkCreateBuffer(renderer.device, &index_buffer_info, nullptr, &index_buffer[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to create a index buffer for rendering. (%d)", result);
			return;
		}

		VkMemoryRequirements vertex_memory_requirements;
		VkMemoryRequirements index_memory_requirements;
		vkGetBufferMemoryRequirements(renderer.device, vertex_buffer[i], &vertex_memory_requirements);
		vkGetBufferMemoryRequirements(renderer.device, index_buffer[i], &index_memory_requirements);

		// Calculate the offset for being properly aligned
		u64 index_offset = draw_list->VtxBuffer.size() * sizeof(ImDrawVert);
		index_offset = index_offset + index_memory_requirements.alignment - (index_offset % index_memory_requirements.alignment);

		VkMemoryAllocateInfo memory_allocation_info = {};
		memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_allocation_info.allocationSize = vertex_memory_requirements.size + index_offset;

		renderer.get_memory_type(vertex_memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocation_info.memoryTypeIndex);

		if ((result = vkAllocateMemory(renderer.device, &memory_allocation_info, nullptr, &buffer_memory[i])) != VK_SUCCESS)
		{
			log(ERROR, "Failed to allocate memory for rendering. (%d)", result);
			return;
		}

		void* data;

		if ((result = vkMapMemory(renderer.device, buffer_memory[i], 0, memory_allocation_info.allocationSize, 0, &data)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to map memory for vertex buffer. (%d)", result);
			return;
		}

		memcpy(data, &draw_list->VtxBuffer.front(), draw_list->VtxBuffer.size() * sizeof(ImDrawVert));
		memcpy((u32*)data + index_offset, &draw_list->IdxBuffer.front(), draw_list->IdxBuffer.size() * sizeof(ImDrawIdx));

		vkUnmapMemory(renderer.device, buffer_memory[i]);

		/*if ((result = vkMapMemory(renderer.device, buffer_memory[i], 0, vertex_memory_requirements.size, 0, &data)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to map memory for vertex buffer. (%d)", result);
			return;
		}

		memcpy(data, &draw_list->VtxBuffer.front(), draw_list->VtxBuffer.size() * sizeof(ImDrawVert));

		vkUnmapMemory(renderer.device, buffer_memory[i]);

		if ((result = vkMapMemory(renderer.device, buffer_memory[i], index_offset, index_memory_requirements.size, 0, &data)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to map memory for index buffer. (%d)", result);
			return;
		}

		memcpy(data, &draw_list->IdxBuffer.front(), draw_list->IdxBuffer.size() * sizeof(ImDrawIdx));

		vkUnmapMemory(renderer.device, buffer_memory[i]);*/

		if ((result = vkBindBufferMemory(renderer.device, vertex_buffer[i], buffer_memory[i], 0)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to bind memory for vertex buffer. (%d)", result);
			return;
		}
		
		if ((result = vkBindBufferMemory(renderer.device, index_buffer[i], buffer_memory[i], index_offset)) != VK_SUCCESS)
		{
			log(ERROR, "Failed to bind memory for index buffer. (%d)", result);
			return;
		}

		u64 offset = 0;
		vkCmdBindVertexBuffers(renderer.command_buffer, 0, 1, &vertex_buffer[i], &offset);
		vkCmdBindIndexBuffer(renderer.command_buffer, index_buffer[i], 0, VK_INDEX_TYPE_UINT16);

		u32 render_offset = 0;

		for (s32 j = 0; j < draw_list->CmdBuffer.size(); j++)
		{
			ImDrawCmd* draw_cmd = &draw_list->CmdBuffer[j];

			if (draw_cmd->UserCallback)
			{
				draw_cmd->UserCallback(draw_list, draw_cmd);
			}
			else
			{
				VkRect2D scissor;
				scissor.offset.x = (s32)draw_cmd->ClipRect.x;
				scissor.offset.y = (s32)(renderer.height - draw_cmd->ClipRect.w);
				scissor.extent.width = renderer.width; // TODO: Fix to proper scissor, once stuff renders
				scissor.extent.height = renderer.height;
				vkCmdSetScissor(renderer.command_buffer, 0, 1, &scissor);

				vkCmdDrawIndexed(renderer.command_buffer, draw_cmd->ElemCount, 1, render_offset, 0, 0);
				//vkCmdDraw(renderer.command_buffer, draw_cmd->ElemCount, 1, 0, 0);
			}

			render_offset += draw_cmd->ElemCount;
		}
	}

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

	if ((result = vkQueueWaitIdle(renderer.queue)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to wait for the queue to become idle. (%d)", result);
		return;
	}

	vkDestroyFramebuffer(renderer.device, framebuffer, nullptr);
	vkDestroySemaphore(renderer.device, semaphore, nullptr);

	for (s32 i = 0; i < draw_data->CmdListsCount; i++)
	{
		vkDestroyBuffer(renderer.device, vertex_buffer[i], nullptr);
		vkDestroyBuffer(renderer.device, index_buffer[i], nullptr);
		vkFreeMemory(renderer.device, buffer_memory[i], nullptr);
	}
}

bool ImGuiVulkanRenderer::prepare_vulkan(void* handle, void* h_instance, u8 device_num)
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
	descriptor_set_layout_binding.binding = 0;
	descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
	descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &descriptor_set_layout_binding;

	if ((result = vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a descriptor set layout. (%d)", result);
		return false;
	}

	// Push constants
	VkPushConstantRange push_constant_ranges[2];
	push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_ranges[0].size = sizeof(float) * 16;
	push_constant_ranges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push_constant_ranges[1].offset = sizeof(float) * 16;
	push_constant_ranges[1].size = sizeof(s32);

	// Create the pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 2;
	pipeline_layout_info.pPushConstantRanges = push_constant_ranges;

	if ((result = vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a pipeline layout. (%d)", result);
		return false;
	}

	// Prepare vertex input bindings
	vertex_input_binding.binding = 0;
	vertex_input_binding.stride = sizeof(ImDrawVert);
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Prepare vertex input attributes
	vertex_input_attribute[0].location = 0;
	vertex_input_attribute[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute[0].offset = offsetof(ImDrawVert, pos.x);

	vertex_input_attribute[1].location = 1;
	vertex_input_attribute[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertex_input_attribute[1].offset = offsetof(ImDrawVert, uv.x);

	vertex_input_attribute[2].location = 2;
	vertex_input_attribute[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertex_input_attribute[2].offset = offsetof(ImDrawVert, col);

	// Create a rendering pipeline
	// Vertex input info
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexAttributeDescriptionCount = 3;
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
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

	ImGuiIO& io = ImGui::GetIO();
	u8* pixels;
	s32 width, height;

	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// Prepare font texture
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.extent = { (u32)width, (u32)height, 1 };
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if ((result = vkCreateImage(device, &image_info, nullptr, &font_image)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a font image. (%d)", result);
		return false;
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, font_image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocation_info = {};
	memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocation_info.allocationSize = memory_requirements.size;

	if (!get_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocation_info.memoryTypeIndex))
	{
		log(ERROR, "Failed to get the memory type. (%d)", result);
		return false;
	}

	if ((result = vkAllocateMemory(device, &memory_allocation_info, nullptr, &device_memory)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to allocate memory for font texture. (%d)", result);
		return false;
	}

	if ((result = vkBindImageMemory(device, font_image, device_memory, 0)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to bind memory for font texture. (%d)", result);
		return false;
	}

	VkSamplerCreateInfo sampler_info;
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = 1;
	sampler_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_info.unnormalizedCoordinates = VK_FALSE;

	if ((result = vkCreateSampler(device, &sampler_info, nullptr, &font_sampler)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create a sampler for font texture. (%d)", result);
		return false;
	}

	VkImageViewCreateInfo image_view_info;
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = font_image;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	image_view_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	if ((result = vkCreateImageView(device, &image_view_info, nullptr, &font_image_view)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to create image view for font texture. (%d)", result);
		return false;
	}
	
	// Update descriptors
	VkDescriptorImageInfo descriptor_image_info = {};
	descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	descriptor_image_info.sampler = font_sampler;
	descriptor_image_info.imageView = font_image_view;

	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set.pImageInfo = &descriptor_image_info;

	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);

	// Upload the image to the GPU
	VkImageSubresource image_subresource = {};
	image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkSubresourceLayout subresource_layout = {};
	vkGetImageSubresourceLayout(device, font_image, &image_subresource, &subresource_layout);

	void* data;

	if ((result = vkMapMemory(device, device_memory, 0, subresource_layout.size, 0, &data)) != VK_SUCCESS)
	{
		log(ERROR, "Failed to map memory for font texture upload. (%d)", result);
		return false;
	}

	memcpy(data, pixels, subresource_layout.size);

	vkUnmapMemory(device, device_memory);

	return true;
}

bool ImGuiVulkanRenderer::initialize(void* handle, void* h_instance, u8 device_num)
{
	// Set some basic ImGui info
	ImGuiIO& io = ImGui::GetIO();
	io.ImeWindowHandle = handle;
	io.RenderDrawListsFn = imgui_render;
	io.UserData = this;

	if (!prepare_vulkan(handle, h_instance, device_num))
	{
		log(ERROR, "Failed to initialize Vulkan renderer.");
		return false;
	}

	return true;
}