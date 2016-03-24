#pragma once

#include "CPU/CPU.h"
#include "HLE/Modules.h"
#include "Loader/Loader.h"
#include "Debugger/Debugger.h"

class Emulator
{
public:
	Emulator();
	bool initialize(std::string path);
	void run();
	void emulate(u64 clock);

	bool running;
	bool emulating;
	bool debugging;

	// For other parts of emulator to access
	std::unique_ptr<Debugger> debugger;
	std::unique_ptr<CPU> cpu;
	E32Image rom;

private:
};

extern Emulator emulator;