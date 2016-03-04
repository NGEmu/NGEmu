#pragma once

union Register
{
	u32 reg;
};

class CPU
{
	friend class Debugger;

public:
	CPU();
	void execute();

private:
	Register registers[0xF];
	Register& LR = registers[0xD];
	Register& SP = registers[0xE];
	u32& PC = registers[0xF].reg;
	Register CPSR;

	s32 breakpoint;
};