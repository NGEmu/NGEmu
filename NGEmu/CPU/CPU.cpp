#include "stdafx.h"
#include "CPU.h"

CPU::CPU()
{
	breakpoint = -1;
	PC = emulator.rom.entry_point;

	// Copy the ROM to memory
	for (u64 i = 0; i < emulator.rom.data.size(); i++)
	{
		memory.write8(static_cast<u32>(i), emulator.rom.data[i]);
	}

	instructions[BRANCH] = &CPU::branch;
}

// Fetch the opcode
inline void CPU::fetch()
{
	jump = 4;
	opcode = memory.read32(PC);
}

// Check the condition and decode the instruction
inline bool CPU::decode()
{
	u8 condition = (opcode & 0xFF) >> 4;
	u8 id = (opcode & 0xF); // The full 4 bit ID of the instruction
	u8 id3 = (opcode & 0xF) >> 1; // First 3 bits of the ID, to help with identification

	switch(condition)
	{
	case 0b1110: // Execute always
		break;

	default:
		log(ERROR, "Unsupported condition: 0x%X", condition);
		return false;
	}

	switch (id3)
	{
	case BRANCH_ID:
		instruction = BRANCH;
		break;

	default:
		log(ERROR, "Unknown instruction: 0x%X", id3);
		return false;
	}

	return true;
}

// Function for debugging checks, commands and such
inline void CPU::debug()
{
	if (breakpoint == PC)
	{
		emulator.emulating = false;
	}
}

void CPU::execute()
{
	// Fetch the opcode
	fetch();
	
	// A false return value indicates for the instruction to be not executed;
	if (!decode())
	{
		PC += jump;
		return;
	}

	// Call the instruction
	(this->*instructions[instruction])();

	// Increment the PC
	PC += jump;

	if (emulator.debugging)
	{
		debug();
	}
}

void CPU::branch()
{
	bool link = (opcode & 0xF) & 1;
	u32 address = (((s32)((opcode >> 8) << 8) >> 8) << 2) + PC + 8;

	if (link)
	{
		log(ERROR, "BRANCH with link is unsupported.");
	}

	PC = address;
	jump = 0;
}