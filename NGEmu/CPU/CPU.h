#pragma once

#include "Memory.h"

// Instructions
enum : u8
{
	// Main opcode IDs
	BRANCH_ID = 0b0101,
	DATA_PROCESSING = 0b0001,
	VARIOUS = 0b0000,

	// Data-processing sub-instructions
	MOVE_ID = 0b1101,

	// Custom stuff
	BRANCH = 0,
	MOVE = 1,
	BRANCH_EXCHANGE = 2,
};
class CPU
{
	friend class Debugger;

public:
	CPU();
	void execute();

private:
	u32 GPR[0xF] = { 1};
	u32& SP = GPR[0xD];
	u32& LR = GPR[0xE];
	u32& PC = GPR[0xF];
	u32 CPSR = {};

	Memory memory;
	s32 breakpoint;
	u32 opcode;
	u32 instruction;
	u8 jump;
	bool thumb;

	void fetch();
	bool decode();
	void debug();

	// Instructions
	void branch();
	void move();
	void branch_exchange();

	// Instruction pointer table
	void (CPU::*instructions[64])();

	// CPSR functions
	inline bool get_N()
	{
		return get_bit(CPSR, 31);
	}

	inline bool get_Z()
	{
		return get_bit(CPSR, 30);
	}

	inline bool get_C()
	{
		return get_bit(CPSR, 29);
	}

	inline bool get_V()
	{
		return get_bit(CPSR, 28);
	}

	inline bool get_T()
	{
		return get_bit(CPSR, 5);
	}

	inline void set_N(bool value)
	{
		set_bit(CPSR, 31, value);
	}

	inline void set_Z(bool value)
	{
		set_bit(CPSR, 30, value);
	}

	inline void set_C(bool value)
	{
		set_bit(CPSR, 29, value);
	}

	inline void set_V(bool value)
	{
		set_bit(CPSR, 28, value);
	}

	inline void set_T(bool value)
	{
		set_bit(CPSR, 5, value);
		thumb = value;
	}
};