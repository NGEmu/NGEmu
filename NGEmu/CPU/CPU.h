#pragma once

#include "Memory.h"

// Instructions
enum : u8
{
	BRANCH_ID = 0b0101,

	// Custom stuff
	BRANCH = 0,
};

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
	Register GPR[0xF] = {};
	u32& LR = GPR[0xD].reg;
	u32& SP = GPR[0xE].reg;
	u32& PC = GPR[0xF].reg;
	u32 CPSR = {};

	Memory memory;
	s32 breakpoint;
	u32 opcode;
	u32 instruction;
	u8 jump;

	void fetch();
	bool decode();
	void debug();

	// Instructions
	void branch();

	// Instruction pointer table
	void (CPU::*instructions[64])();

	// CPSR functions
	inline bool get_N()
	{
		return (CPSR >> 31) & 1;
	}

	inline bool get_Z()
	{
		return (CPSR >> 30) & 1;
	}

	inline bool get_C()
	{
		return (CPSR >> 29) & 1;
	}

	inline bool get_V()
	{
		return (CPSR >> 28) & 1;
	}

	inline void set_N(bool value)
	{
		if (value)
		{
			CPSR |= 1 << 31;
		}
		else
		{
			CPSR &= ~(1 << 31);
		}
	}

	inline void set_Z(bool value)
	{
		if (value)
		{
			CPSR |= 1 << 30;
		}
		else
		{
			CPSR &= ~(1 << 30);
		}
	}

	inline void set_C(bool value)
	{
		if (value)
		{
			CPSR |= 1 << 29;
		}
		else
		{
			CPSR &= ~(1 << 29);
		}
	}

	inline void set_V(bool value)
	{
		if (value)
		{
			CPSR |= 1 << 28;
		}
		else
		{
			CPSR &= ~(1 << 28);
		}
	}
};