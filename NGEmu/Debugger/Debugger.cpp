#include "stdafx.h"
#include "Debugger.h"

s64 Debugger::window_callback(HWND& handle, u32& msg, u64& param1, s64& param2)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		stop();
		break;
	}
	}

	return 0xC0FFEE;
}

Debugger::Debugger(std::function<void()> pause_callback, std::function<void()> stop_callback)
{
	renderer.reset(new ImGuiVulkanRenderer());
	window.reset(new Window(std::bind(&Debugger::window_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
	pause = pause_callback;
	stop = stop_callback;
}

bool Debugger::initialize()
{
	if (!window->create_window("Debugger", "Debugger", 1100, 800, WINDOW, NORMAL))
	{
		return false;
	}

	if (!renderer->initialize(window->get_handle(), window->get_instance()))
	{
		log(ERROR, "ImGui Vulkan renderer failed to initialize.");
		return false;
	}

	return true;
}

void Debugger::render()
{
	renderer->new_frame(1100, 800);
	ImGui::ShowTestWindow();
	ImGui::Render();
}