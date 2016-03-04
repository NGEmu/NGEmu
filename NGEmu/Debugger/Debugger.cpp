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

	case WM_DESTROY:
		emulator.running = false;
		opened = false;
		break;
	}

	return DEFAULT_PROCEDURE;
}

Debugger::Debugger()
{
	renderer.reset(new ImGuiVulkanRenderer());
	window.reset(new Window(std::bind(&Debugger::window_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
	opened = false;

	// Set up the ImGui keymap
	ImGuiIO& io = ImGui::GetIO();

#ifdef _WIN32
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
#endif

	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';
}

Debugger::~Debugger()
{
	emulator.debugging = false;
	emulator.cpu->breakpoint = -1;
}

bool Debugger::initialize()
{
	if (!window->create_window("Debugger", "Debugger", 1280, 830, WINDOW, NORMAL))
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

	emulator.debugging = true;
	emulator.emulating = false;

	return true;
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

	// TODO: Go to address
	//ImGui::InputText("", buffer, 5, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

	// The scrolling needs to be performed later, once we have already created the disassembly window
	bool scroll_to_pc = false;

	if (ImGui::Button("Go to PC"))
	{
		scroll_to_pc = true;
	}

	ImGui::EndChild();
	ImGui::BeginChild("Disassembly", ImVec2(592, 760), true);
	
	ImGui::Columns(4, "Disassembly");
	ImGui::SetColumnOffset(1, 26);
	ImGui::SetColumnOffset(2, 90);
	ImGui::SetColumnOffset(3, 182);
	ImGui::Text(""); ImGui::NextColumn(); // To indicate a breakpoint
	ImGui::Text("Address"); ImGui::NextColumn();
	ImGui::Text("Bytes"); ImGui::NextColumn();
	ImGui::Text("Instruction"); ImGui::NextColumn();
	ImGui::Separator();

	ImGuiListClipper clipper(0x380000 / 4, ImGui::GetTextLineHeight()); // Bytes are grouped by four (the alignment for instructions)
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImColor breakpoint_fill = ImColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	ImColor breakpoint_border = ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImVec2 screen_cursor = ImGui::GetCursorScreenPos();

	// Perform scrolling, if necessary
	if (scroll_to_pc)
	{
		ImGui::SetScrollFromPosY(emulator.cpu->PC * ImGui::GetTextLineHeight());
	}

	for (s32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
	{
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
		ImGui::Text("DE AD B0 0B", i); ImGui::NextColumn();
		ImGui::Text("MOV R0, #0"); ImGui::NextColumn();
	}

	clipper.End();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Registers", ImVec2(200, 280), true);

	for (u8 i = 0; i < 13; i++)
	{
		ImGui::Text("R%d: 0x%X", i, emulator.cpu->registers[i]);
	}

	ImGui::Text("SP: 0x%X", emulator.cpu->SP);
	ImGui::Text("LR: 0x%X", emulator.cpu->LR);
	ImGui::Text("PC: 0x%X", emulator.cpu->PC);

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