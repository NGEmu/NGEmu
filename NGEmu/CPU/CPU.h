#pragma once

#include "Memory.h"

// Instructions
enum : u8
{
	// Main opcode IDs
	VARIOUS         = 0b0000,
	DATA_PROCESSING = 0b0010,
	MULTIPLE_REG    = 0b1000,
	BRANCH_ID       = 0b1010,

	// Data-processing sub-instructions
	SUBTRACT_ID = 0b0010,
	MOVE_ID     = 0b1101,

	// Various opcode sub-categories
	MISCELLANEOUS_ID = 0b0010,

	// Miscellaneous categories
	MISCELLANEOUS_STATUS_REGISTER = 0b0000,
	MISCELLANEOUS_BRANCH_ZEROS    = 0b0001,
	MISCELLANEOUS_BRANCH_LINK     = 0b0011,
	MISCELLANEOUS_DSP_ADD_SUB     = 0b0101,
	MISCELLANEOUS_DSP_MULT        = 0b1000,

	// Miscellaneous opcodes
	MISCELLANEOUS_OTHER = 0b0001,
	MISCELLANEOUS_ZEROS = 0b0011,

	// Multiple register adressing modes
	DECREMENT_AFTER  = 0b0000,
	INCREMENT_AFTER  = 0b0001,
	DECREMENT_BEFORE = 0b0010,
	INCREMENT_BEFORE = 0b0011,

	// Instruction table IDs
	BRANCH          = 0,
	BRANCH_EXCHANGE = 1,
	MOVE            = 2,
	MULTIPLE        = 3,
	SUBTRACT        = 4,
};
class CPU
{
	friend class Debugger;

public:
	CPU();
	void execute();

private:
	u32 GPR[0xF] = {};
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

	void fetch(u8 default_jump);
	bool decode_ARM();
	bool decode_Thumb();
	void debug();

	// Instructions
	void branch();
	void branch_exchange();
	void move();
	void multiple();
	void subtract();

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