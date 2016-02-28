#include "stdafx.h"
#include "Debugger.h"

s64 Debugger::window_callback(HWND& handle, u32& msg, u64& param1, s64& param2)
{
	ImGuiIO& io = ImGui::GetIO();

	switch (msg)
	{
	case WM_MOUSEMOVE:
		io.MousePos.x = static_cast<float>(param2 & 0xFFFF);
		io.MousePos.y = static_cast<float>(param2 >> 16);
		break;

	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		break;

	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		break;

	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		break;

	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		break;

	case WM_DESTROY:
		stop();
		opened = false;
		break;
	}

	return DEFAULT_PROCEDURE;
}

Debugger::Debugger(std::function<void()> pause_callback, std::function<void()> stop_callback)
{
	renderer.reset(new ImGuiVulkanRenderer());
	window.reset(new Window(std::bind(&Debugger::window_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
	pause = pause_callback;
	stop = stop_callback;
	opened = false;
}

bool Debugger::initialize()
{
	if (!window->create_window("Debugger", "Debugger", 1100, 800, WINDOW, NORMAL))
	{
		return false;
	}

	opened = true;

	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.1f;
	clear_value.color.float32[1] = 0.075f;
	clear_value.color.float32[2] = 0.025f;

	ImGuiVulkanOptions options;
	options.clear_value = clear_value;
	options.device_number = 0;
	options.validation_layers = false;

	if (!renderer->initialize(window->get_handle(), window->get_instance(), options))
	{
		log(ERROR, "ImGui Vulkan renderer failed to initialize.");
		return false;
	}

	return true;
}

void Debugger::render()
{
	// Sometimes when the main loop is being killed, this still gets called, so we need to exit if the loop was killed
	if (!opened)
	{
		return;
	}

	renderer->new_frame();
	ImGui::ShowTestWindow();
	ImGui::Render();
}