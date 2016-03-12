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

	instructions[BRANCH]          = &CPU::branch;
	instructions[MOVE]            = &CPU::move;
	instructions[BRANCH_EXCHANGE] = &CPU::branch_exchange;
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

	log(DEBUG, "0x%X", opcode);

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
		u8 id2 = ((opcode >> 15) & 1) | ((opcode & 1) << 1);

		if (id2 == 0b0010 && !((opcode >> 12) & 1) && !((opcode >> 4) & 1))
		{
			u8 operation = opcode >> 28;
			u8 operation_minor = (opcode >> 13) & 3;

			switch (operation)
			{
			case 0b0001:
			{
				if (operation_minor == 0b0011)
				{
					log(DEBUG, "Count leading zeros unimplemented.");
				}
				else
				{
					instruction = BRANCH_EXCHANGE;
				}
			}
			break;

			default:
				log(ERROR, "Unknown miscellaneous operation");
				return false;
			}
		}
		else
		{
			log(ERROR, "Unknown 0b000 instruction.");
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
		u8 sub_opcode = (opcode >> 13) | ((opcode & 1) << 3);

		switch (sub_opcode)
		{
		case MOVE_ID:
		{
			instruction = MOVE;
		}
		break;

		default:
			log(ERROR, "Unimplemented data processing instruction. (0x%X)", sub_opcode);
		}
	}
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

void CPU::move()
{
	bool update = (opcode >> 12) & 1;
	u16 operand = opcode >> 24;
	u8 SBZ = (opcode >> 20) & 0xF; // Should be zero
	u8 Rd = (opcode >> 16) & 0xF;

	if (!update && ((operand >> 8) & 1) && ((operand >> 11) & 1))
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
	log(DEBUG, "Branch exchange");
	u8 Rm = (opcode >> 24) & 0xF;
	set_T(GPR[Rm] & 1);

	if (Rm == 15)
	{
		log(ERROR, "Register 15 specified for branch exchange");
	}

	PC = GPR[Rm] & 0xFFFFFFFE;
	jump = 0;
}