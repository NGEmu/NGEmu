#pragma once

struct E32Image
{
	// Emulator specific stuff
	std::vector<u8> data; // Vector of bytes of the ROM
	u32 entry_point;      // Entry point address

	// Header
	u32 uid1;
	u32 uid2;
	u32 uid3;
	u32 uid_checksum;
	u32 signature;
	u32 cpu;
	u32 code_checksum;
	u32 data_checksum;
	u8 major;
	u8 minor;
	u16 build;
	s64 timestamp;
	u32 flags;
	s32 code_size;
	s32 data_size;
	s32 heap_minimum_size;
	s32 heap_maximum_size;
	s32 stack_size;
	s32 BSS_size;
	u32 entry_point_offset;
	u32 code_base_address;
	u32 data_base_address;
	s32 dll_count;
	u32 export_offset;
	s32 export_count;
	s32 text_size;
	u32 code_offset;
	u32 data_offset;
	u32 import_offset;
	u32 code_relocation_offset;
	u32 data_relocation_offset;
	u32 priority;

	// Only for V-formatted headers
	u32 uncompressed_size;
	// TODO: Security information (0x10)
	u32 exception_descriptor_offset; // Bit 0 is clear if not used
	u32 unused;
	u32 bitmap_holes_size;
	u32 bitmap_holes_format;
	// ... Bitmap describing holes in export tables (variable length)

	// Flags
	bool executable_type;  // false = executable, true = DLL
	bool call_entry_point; // false = call, true = don't call
	bool fixed_address;    // 0 = not fixed address, 1 = fixed address
	u8 abi;                // 0 = GCC98r2, 1 = EABI
	u8 entry_point_type;   // 0 = EKA1, 1 = EKA2
	u8 header_format;      // 0 = Basic, 1 = J-format, 2 = V-format
	u8 import_format;      // 0 = Standard PE format, 1 = ELF format, 2 = PE format with some other custom shit
};

namespace loader
{
	bool load(std::string path, E32Image& image);
	bool parse(E32Image& image);

	// For checksums
	u16 crc16_ccitt(u8 (&values)[6]);
	void u8_from_32(u8 output[4], u32 value);
}