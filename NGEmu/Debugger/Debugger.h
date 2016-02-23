#pragma once

#include "ImGuiVulkanRenderer.h"

class Debugger
{
public:
	Debugger(std::function<void()> pause_callback, std::function<void()> stop_callback);
	bool initialize();
	void render();

	std::unique_ptr<Window> window;

private:
	s64 window_callback(HWND& handle, u32& msg, u64& param1, s64& param2);
	std::unique_ptr<ImGuiVulkanRenderer> renderer;

	// Emulator callbacks
	std::function<void()> pause;
	std::function<void()> stop;
};