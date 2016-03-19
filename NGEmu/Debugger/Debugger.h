#pragma once

#define REPLACE_LOGGER
#include "ImGuiRenderers.h"

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
	std::unique_ptr<ImGuiRenderer> renderer;

	void display_debugger();
	std::string get_register_name(u8 reg);
	std::string parse_register_list(u16 reg_list);
	std::string parse_instruction(u32 opcode, u32 PC);
	std::string parse_thumb_instruction(u16 opcode, u32 PC);

	bool opened;
	bool track_pc = true;
};