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

	case WM_MOUSEWHEEL:
		io.MouseWheel += (s16)((param1 >> 16) & 0xFFFF) > 0 ? 1.0f : -1.0f;
		break;

	case WM_KEYDOWN:
		if (param1 < 256)
		{
			io.KeysDown[param1] = true;
		}
		break;

	case WM_KEYUP:
		if (param1 < 256)
		{
			io.KeysDown[param1] = false;
		}
		break;

	case WM_CHAR:
		if (param1 > 0 && param1 < 0x10000)
		{
			io.AddInputCharacter((u8)param1);
		}
		break;

	case WM_DESTROY:
		emulator.running = false;
		opened = false;
		ImGui::Shutdown();
		break;
	}

	return DEFAULT_PROCEDURE;
}

Debugger::Debugger()
{
	window.reset(new Window(std::bind(&Debugger::window_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
	opened = false;
}

Debugger::~Debugger()
{
	emulator.debugging = false;
}

bool Debugger::initialize()
{
	renderer.reset(new ImGuiVulkanRenderer());

	if (!window->create_window("Debugger", "Debugger", 1280, 830, WINDOW, NORMAL))
	{
		return false;
	}

	opened = true;

	VkClearValue clear_value;
	clear_value.color.float32[0] = 0.1f;
	clear_value.color.float32[1] = 0.075f;
	clear_value.color.float32[2] = 0.025f;

	ImGuiVulkanOptions vulkan_options;
	vulkan_options.clear_value = clear_value;
	vulkan_options.device_number = 0;
	vulkan_options.validation_layers = false;
	vulkan_options.use_precompiled_shaders = true;

	if (!renderer->initialize(window->get_handle(), window->get_instance(), &vulkan_options))
	{
		log(ERROR, "ImGui Vulkan renderer failed to initialize.");
		return false;
	}

	emulator.debugging = true;
	emulator.emulating = false;

	return true;
}

std::string Debugger::get_register_name(u8 reg)
{
	switch (reg)
	{
	case 0xD:
		return "SP";
		
	case 0xE:
		return "LR";

	case 0xF:
		return "PC";
	}

	return format("R%d", reg);
}

std::string Debugger::parse_instruction(u32 opcode, u32 PC)
{
	u8 condition = (opcode >> 28) & 0xF;
	u8 id = (opcode >> 24) & 0xF; // The full 4 bit ID of the instruction
	u8 id3 = (opcode >> 24) & 0xE; // First 3 bits of the ID, to help with identification

	if (condition == 0b1111)
	{
		return "0b1111 condition";
	}

	switch (id3)
	{
	case VARIOUS:
	{
		u8 id2 = (opcode >> 23) & 3;
		bool update = (opcode >> 20) & 1;
		
		if (!update && id2 == MISCELLANEOUS_ID)
		{
			u8 id4 = (opcode >> 4) & 0xF;
			u8 op = (opcode >> 21) & 3;

			if (id4 == MISCELLANEOUS_BRANCH_ZEROS)
			{
				if (op == MISCELLANEOUS_OTHER) // Branch and exchange
				{
					u8 Rm = opcode & 0xF;
					return format("BX %s", get_register_name(Rm));
				}
				else
				{
					return "Unknown misc other";
				}
			}
			else
			{
				return "Unknown misc category";
			}
		}
		else
		{
			return "Unknown various op";
		}
	}
	break;

	case BRANCH_ID:
	{
		bool link = id & 1;
		u32 address = (((s32)(opcode << 8) >> 8) << 2) + PC + 8;

		if (link)
		{
			return format("BL %X", address);
		}
		else
		{
			return format("B %X", address);
		}
	}
	break;

	case DATA_PROCESSING:
	{
		u8 sub_opcode = (opcode >> 21) & 0xF;

		switch (sub_opcode)
		{
		case MOVE_ID:
		{
			u8 Rd = (opcode >> 12) & 0xF;
			u16 operand = opcode & 0xFFF;

			return format("MOV %s, #%X", get_register_name(Rd), operand & 0xF);
		}
		break;
		}
	}
	break;
	}

	return "Unknown";
}

std::string Debugger::parse_thumb_instruction(u16 opcode, u32 PC)
{
	return "Unknown";
}

void Debugger::display_debugger()
{
	bool display_disassembly = true;
	bool display_registers = true;
	u32 debugger_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::Begin("Disassembly, controls", &display_disassembly, debugger_flags);
	ImGui::BeginChild("Controls", ImVec2(800, 35), true);

	// Controls
	if (ImGui::Button("Run"))
	{
		emulator.emulating = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Pause"))
	{
		emulator.emulating = false;
	}

	ImGui::SameLine();

	if (ImGui::Button("Step"))
	{
		emulator.cpu->execute();
	}

	ImGui::SameLine();

	// The scrolling needs to be performed later, once we have already created the disassembly window
	bool scroll_to_pc = false;

	if (ImGui::Button("Go to PC"))
	{
		scroll_to_pc = true;
	}

	ImGui::SameLine();

	ImGui::Checkbox("Track PC", &track_pc);

	ImGui::EndChild();
	ImGui::BeginChild("Disassembly", ImVec2(592, 760), true);
	
	ImGui::Columns(4, "Disassembly");
	ImGui::SetColumnOffset(1, 26);
	ImGui::SetColumnOffset(2, 96);
	ImGui::SetColumnOffset(3, 188);
	ImGui::Text(""); ImGui::NextColumn(); // To indicate a breakpoint
	ImGui::Text("Address"); ImGui::NextColumn();
	ImGui::Text("Bytes"); ImGui::NextColumn();
	ImGui::Text("Instruction"); ImGui::NextColumn();
	ImGui::Separator();

	u8 instruction_bytes = emulator.cpu->thumb ? 2 : 4;

	ImGuiListClipper clipper(0x380000 / instruction_bytes, ImGui::GetTextLineHeight()); // Bytes are grouped by four (the alignment for instructions)
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImColor breakpoint_fill = ImColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	ImColor breakpoint_border = ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImVec2 screen_cursor = ImGui::GetCursorScreenPos();

	// Perform scrolling, if necessary
	if (track_pc || scroll_to_pc)
	{
		ImGui::SetScrollFromPosY(((emulator.cpu->PC / instruction_bytes) * ImGui::GetTextLineHeight()) - ImGui::GetScrollY(), 0.35f);
	}

	for (s32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
	{
		s32 clipper_i = i;
		i *= instruction_bytes;

		screen_cursor = ImGui::GetCursorScreenPos();

		if (ImGui::Selectable("", emulator.cpu->PC == i, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
		{
			emulator.cpu->breakpoint = i;
		}

		if (emulator.cpu->breakpoint == i)
		{
			// We use custom rendering for drawing the breakpoint
			draw_list->AddCircleFilled(ImVec2(screen_cursor.x + 6, screen_cursor.y + 7), 7, breakpoint_fill);
			draw_list->AddCircle(ImVec2(screen_cursor.x + 6, screen_cursor.y + 7), 7, breakpoint_border);
		}

		ImGui::NextColumn();
		ImGui::Text("0x%X", i); ImGui::NextColumn();
		ImGui::Text("%02X %02X %02X %02X", emulator.cpu->memory.read8(i + 3), emulator.cpu->memory.read8(i + 2), emulator.cpu->memory.read8(i + 1), emulator.cpu->memory.read8(i));
		ImGui::NextColumn();

		if (emulator.cpu->thumb)
		{
			ImGui::Text("%s", parse_thumb_instruction(emulator.cpu->memory.read16(i), i).c_str());
		}
		else
		{
			ImGui::Text("%s", parse_instruction(emulator.cpu->memory.read32(i), i).c_str());
		}

		ImGui::NextColumn();

		i = clipper_i;
	}

	clipper.End();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Registers", ImVec2(200, 300), true);

	for (u8 i = 0; i < 13; i++)
	{
		ImGui::Text("R%d: 0x%X", i, emulator.cpu->GPR[i]);
	}

	ImGui::Text("SP: 0x%X", emulator.cpu->SP);
	ImGui::Text("LR: 0x%X", emulator.cpu->LR);
	ImGui::Text("PC: 0x%X", emulator.cpu->PC);
	ImGui::Text("CPSR: 0x%X", emulator.cpu->CPSR);

	ImGui::EndChild();
	ImGui::End();
}

void Debugger::render()
{
	renderer->new_frame();

	// Sometimes when the main loop is being killed, this still gets called, so we need to exit if the loop was killed
	if (!opened)
	{
		return;
	}

	display_debugger();
	ImGui::Render();
}