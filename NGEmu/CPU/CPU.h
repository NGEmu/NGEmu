#pragma once

#include "Memory.h"

// Some helper functions
u16 parse_operand(u16 operand);

// Instructions
enum : u8
{
	// Conditions
	EQUAL  = 0b0000,
	ALWAYS = 0b1110,

	// Main opcode IDs
	VARIOUS             = 0b0000,
	DATA_PROCESSING     = 0b0010,
	IMMEDIATE_OFFSET_ID = 0b0100,
	MULTIPLE_REG        = 0b1000,
	BRANCH_ID           = 0b1010,

	// Data-processing sub-instructions
	SUBTRACT_ID = 0b0010,
	ADD_ID      = 0b0100,
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
	BRANCH           = 0,
	BRANCH_EXCHANGE  = 1,
	MOVE             = 2,
	MULTIPLE         = 3,
	SUBTRACT         = 4,
	ADD              = 5,
	IMMEDIATE_OFFSET = 6,
};

struct CPSR
{
	u32 M : 4;
	u32 T : 1;
	u32 F : 1;
	u32 I : 1;
	u32   : 20;
	u32 Q : 1;
	u32 V : 1;
	u32	C : 1;
	u32 Z : 1;
	u32 N : 1;
};

class CPU
{
public:
	CPU();
	void execute();

	u32 GPR[0x10] = {};
	u32& SP = GPR[0xD];
	u32& LR = GPR[0xE];
	u32& PC = GPR[0xF];
	CPSR CPSR = {};

	Memory memory;
	s32 breakpoint;
	u32 opcode;
	u32 instruction;
	u8 jump;

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
	void add();
	void immediate_offset();

	// Instruction pointer table
	void (CPU::*instructions[64])();
};