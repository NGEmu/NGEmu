#include "stdafx.h"
#include "CPU.h"

// Helper functions
u16 parse_operand(u16 operand)
{
	u8 rotate = (operand >> 8) * 2;
	u16 immediate = rotate_right((u16)(operand & 0xFF), rotate);
	return immediate;
}

CPU::CPU()
{
	breakpoint = -1;
	PC = emulator.rom.entry_point;
	SP = 0x1000000; // This might not be correct

	// Copy the ROM to memory
	for (u64 i = 0; i < emulator.rom.data.size(); i++)
	{
		memory.write8(static_cast<u32>(i), emulator.rom.data[i]);
	}

	instructions[BRANCH]           = &CPU::branch;
	instructions[BRANCH_EXCHANGE]  = &CPU::branch_exchange;
	instructions[MOVE]             = &CPU::move;
	instructions[MULTIPLE]         = &CPU::multiple;
	instructions[SUBTRACT]         = &CPU::subtract;
	instructions[ADD]              = &CPU::add;
	instructions[IMMEDIATE_OFFSET] = &CPU::immediate_offset;
}

// Fetch the opcode
inline void CPU::fetch(u8 default_jump)
{
	jump = default_jump;
	opcode = memory.read32(PC);
}

// Check the condition and decode the instruction (A false return value indicates for the instruction to be not executed)
bool CPU::decode_ARM()
{
	u8 condition = (opcode >> 28) & 0xF;
	u8 id3 = (opcode >> 24) & 0xE; // First 3 bits of the ID, to help with identification

	// Check the execution condition
	switch(condition)
	{
	case 0b1110: // Always (Unconditional execution)
		break;

	default:
		log(ERROR, "Unsupported condition: 0x%X", condition);
		return false;
	}

	switch (id3)
	{
	case VARIOUS:
	{
		u8 id2 = (opcode >> 23) & 3;
		bool update = (opcode >> 20) & 1;

		if (!update && id2 == MISCELLANEOUS_ID)
		{
			u8 id4 = (opcode >> 4) & 0xF;
			u8 op = (opcode >> 21) & 3;

			if (id4 == MISCELLANEOUS_BRANCH_ZEROS)
			{
				if (op == MISCELLANEOUS_OTHER) // Branch and exchange
				{
					instruction = BRANCH_EXCHANGE;
				}
				else
				{
					log(ERROR, "Unknown misc other");
					return false;
				}
			}
			else
			{
				log(ERROR, "Unknown misc category");
				return false;
			}
		}
		else
		{
			log(ERROR, "Unknown various op");
			return false;
		}
	}
	break;

	case DATA_PROCESSING:
	{
		u8 sub_opcode = (opcode >> 21) & 0xF;

		switch (sub_opcode)
		{
		case SUBTRACT_ID:
		{
			instruction = SUBTRACT;
		}
		break;

		case ADD_ID:
		{
			instruction = ADD;
		}
		break;

		case MOVE_ID:
		{
			instruction = MOVE;
		}
		break;

		default:
			log(ERROR, "Unimplemented data processing instruction. (0x%X)", sub_opcode);
			return false;
		}
	}
	break;

	case IMMEDIATE_OFFSET_ID:
	{
		instruction = IMMEDIATE_OFFSET;
	}
	break;

	case MULTIPLE_REG:
	{
		instruction = MULTIPLE;
	}
	break;

	case BRANCH_ID:
	{
		instruction = BRANCH;
	}
	break;

	default:
		log(ERROR, "Unknown instruction: 0x%X", id3);
		return false;
	}

	return true;
}

// Decode the instruction
bool CPU::decode_Thumb()
{
	log(DEBUG, "Decoding Thumb");
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
	// Whether to execute as Thumb or ARM
	if (thumb)
	{
		fetch(2);

		if (!decode_Thumb())
		{
			PC += jump;
			return;
		}
	}
	else
	{
		fetch(4);

		if (!decode_ARM())
		{
			PC += jump;
			return;
		}
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
	bool link = (opcode >> 24) & 1;
	u32 address = (((s32)(opcode << 8) >> 8) << 2) + PC + 8;

	if (link)
	{
		LR = PC + 4;
	}

	PC = address;
	jump = 0;
}

void CPU::branch_exchange()
{
	u8 Rm = opcode & 0xF;
	jump = 0;

	if (Rm == 12) // System call
	{
		u8 module = GPR[Rm] >> 24;
		u32 ordinal = GPR[Rm] & 0xFFFFFF;
		HLE::call_HLE(module, ordinal);
		PC = LR;
		return;
	}
	else if (Rm == 15)
	{
		log(ERROR, "Register 15 specified for branch exchange");
		return;
	}

	set_T(GPR[Rm] & 1);
	PC = GPR[Rm] & 0xFFFFFFFE;
}

void CPU::move()
{
	bool update = (opcode >> 20) & 1;
	u8 SBZ = (opcode >> 16) & 0xF; // Should be zero
	u8 Rd = (opcode >> 12) & 0xF;
	u16 operand = opcode & 0xFFF;

	if (!((update >> 25) & 1) && ((operand >> 7) & 1) && ((operand >> 4) & 1))
	{
		log(ERROR, "Extended MOV instruction!");
		return;
	}

	if (SBZ != 0)
	{
		log(ERROR, "MOV SBZ is not zero!");
	}

	GPR[Rd] = parse_operand(operand);

	if (update && Rd == 15)
	{
		log(ERROR, "MOV with S == 1 and Rd == 15");
	}
	else if (update)
	{
		log(ERROR, "MOV with S == 1");
	}
}

void CPU::multiple()
{
	u8 mode = (opcode >> 23) & 3;
	bool S = (opcode >> 22) & 1;
	bool W = (opcode >> 21) & 1;
	bool L = (opcode >> 20) & 1;
	u8 Rn = (opcode >> 16) & 0xF;
	u16 register_list = opcode & 0xFFFF;

	if (L)
	{
		log(ERROR, "Multiple load unsupported");
		return;
	}

	if (S)
	{
		log(ERROR, "Multiple update unsupported");
		return;
	}

	u8 registers = 0;

	for (u8 i = 0; i < 0xF; i++)
	{
		if ((register_list >> i) & 1)
		{
			registers++;
		}
	}

	u32 address = GPR[Rn];

	if (mode == DECREMENT_BEFORE)
	{
		address -= registers * 4;
	}
	else
	{
		log(ERROR, "Unsupported multiple addressing mode");
		return;
	}

	for (u8 i = 0; i < 0xF; i++)
	{
		if ((register_list >> i) & 1)
		{
			memory.write32(address, GPR[i]);
			address += 4;
		}
	}

	if (W)
	{
		GPR[Rn] -= registers * 4;
	}
	else
	{
		log(ERROR, "Multiple base register decrementation not supported");
	}
}

void CPU::subtract()
{
	bool update = (opcode >> 20) & 1;
	u8 Rn = (opcode >> 16) & 0xF;
	u8 Rd = (opcode >> 12) & 0xF;
	
	GPR[Rd] = GPR[Rn] - parse_operand(opcode & 0xFFF);

	if (update && Rd == 15)
	{
		log(ERROR, "MOV with S == 1 and Rd == 15");
	}
	else if (update)
	{
		log(ERROR, "MOV with S == 1");
	}
}

void CPU::add()
{
	bool update = (opcode >> 20) & 1;
	u8 Rn = (opcode >> 16) & 0xF;
	u8 Rd = (opcode >> 12) & 0xF;

	GPR[Rd] = GPR[Rn] + parse_operand(opcode & 0xFFF);

	if (update && Rd == 15)
	{
		log(ERROR, "MOV with S == 1 and Rd == 15");
	}
	else if (update)
	{
		log(ERROR, "MOV with S == 1");
	}
}

void CPU::immediate_offset()
{
	bool P = (opcode >> 24) & 1;
	bool U = (opcode >> 23) & 1;
	bool B = (opcode >> 22) & 1;
	bool W = (opcode >> 21) & 1;
	bool L = (opcode >> 20) & 1;
	u8 Rn = (opcode >> 16) & 0xF;
	u8 Rd = (opcode >> 12) & 0xF;
	u16 offset = opcode & 0xFFF;

	if (!L)
	{
		log(ERROR, "STR unsupported");
		return;
	}

	if (B)
	{
		log(ERROR, "LDRB/STRB unsupported");
		return;
	}

	u32 address = GPR[Rn];

	// Check for PC
	if (Rn == 0xF)
	{
		address += 8;
	}

	if (U)
	{
		address += offset;
	}
	else
	{
		address -= offset;
	}

	GPR[Rd] = memory.read32(address);

	if (P && W)
	{
		GPR[Rn] = address;
	}
}