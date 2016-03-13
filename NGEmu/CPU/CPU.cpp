#include "stdafx.h"
#include "CPU.h"

CPU::CPU()
{
	breakpoint = -1;

	// Copy the ROM to memory (don't copy the header)
	for (u64 i = 0; i < emulator.rom.data.size(); i++)
	{
		memory.write8(static_cast<u32>(i), emulator.rom.data[i]);
	}

	instructions[BRANCH]          = &CPU::branch;
	instructions[MOVE]            = &CPU::move;
	instructions[BRANCH_EXCHANGE] = &CPU::branch_exchange;
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

	case BRANCH_ID:
	{
		instruction = BRANCH;
	}
	break;

	case DATA_PROCESSING:
	{
		u8 sub_opcode = (opcode >> 21) & 0xF;

		switch (sub_opcode)
		{
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

	default:
		log(ERROR, "Unknown instruction: 0x%X", id3);
		return false;
	}

	return true;
}

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
		log(ERROR, "BRANCH with link is unsupported.");
	}

	PC = address;
	jump = 0;
}

void CPU::move()
{
	bool update = (opcode >> 20) & 1;
	u16 operand = opcode & 0xFFF;
	u8 SBZ = (opcode >> 16) & 0xF; // Should be zero
	u8 Rd = (opcode >> 12) & 0xF;

	if (!((update >> 25) & 1) && ((operand >> 7) & 1) && ((operand >> 4) & 1))
	{
		log(ERROR, "Extended MOV instruction!");
	}

	GPR[Rd] = operand & 0xFF;

	if (update && Rd == 15)
	{
		log(ERROR, "MOV with S == 1 and Rd == 15");
	}
	else if (update)
	{
		log(ERROR, "MOV with S == 1");
	}
}

void CPU::branch_exchange()
{
	u8 Rm = (opcode >> 24) & 0xF;
	set_T(GPR[Rm] & 1);

	if (thumb)
	{
		log(DEBUG, "Switched to Thumb");
	}
	else
	{
		log(DEBUG, "Still on ARM mode");
	}

	if (Rm == 15)
	{
		log(ERROR, "Register 15 specified for branch exchange");
	}

	PC = GPR[Rm] & 0xFFFFFFFE;
	jump = 0;
}