#include "stdafx.h"
#include "CPU.h"

CPU::CPU()
{
	breakpoint = -1;

	// Reset the register
	for (u8 i = 0; i < 0xF; i++)
	{
		registers[i].reg = 0;
	}

	PC = 0x100;
}

void CPU::execute()
{
	log(DEBUG, "Executing");
	PC += 4;

	if (breakpoint == PC)
	{
		emulator.emulating = false;
	}
}