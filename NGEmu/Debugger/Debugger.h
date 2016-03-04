#pragma once

#define REPLACE_LOGGER
#include "ImGuiVulkanRenderer.h"

class Debugger
{
public:
	Debugger();
	~Debugger();
	bool initialize();
	void render();

	std::unique_ptr<Window> window;

private:
	s64 window_callback(HWND& handle, u32& msg, u64& param1, s64& param2);
	std::unique_ptr<ImGuiVulkanRenderer> renderer;
	bool opened;

	void display_debugger();
};