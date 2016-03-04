#pragma once

#include "Debugger/Debugger.h"
#include "CPU/CPU.h"

class Emulator
{
public:
	Emulator();
	bool initialize();
	void run();
	void emulate(u64 clock);

	bool running;
	bool emulating;
	bool debugging;

	// For other parts of emulator to access
	std::unique_ptr<Debugger> debugger;
	std::unique_ptr<CPU> cpu;

private:
};

extern Emulator emulator;