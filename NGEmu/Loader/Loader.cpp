#include "stdafx.h"
#include "Loader.h"

bool loader::load(std::string path, E32Image& image)
{
	std::ifstream stream(path, std::ios::binary);

	if (!stream)
	{
		log(ERROR, "Failed to open image file.");
		return false;
	}

	stream.seekg(0, std::ios::end);
	u64 length = stream.tellg();
	stream.seekg(0, std::ios::beg);

	image.data.resize(length);

	for (u64 i = 0; i < length; i++)
	{
		image.data[i] = stream.get();
	}

	stream.close();
	parse(image);

	return true;
}

u16 loader::crc16_ccitt(u8 (&values)[6])
{
	u32 result = 0;

	for (u8 b : values)
	{
		result ^= (b << 8);

		for (u8 d = 0; d < 8; d++)
		{
			result = result << 1;

			if (result & 0x10000)
			{
				result ^= 0x1021;
			}

			result &= 0xffff;
		}
	}

	return result;
}

void loader::u8_from_32(u8 output[4], u32 value)
{
	output[0] = (u8)value;
	output[1] = (u8)(value >>= 8);
	output[2] = (u8)(value >>= 8);
	output[3] = (u8)(value >>= 8);
}

bool loader::parse(E32Image& image)
{
	u32* data32 = reinterpret_cast<u32*>(image.data.data());

	image.uid1 = data32[0];
	image.uid2 = data32[1];
	image.uid3 = data32[2];
	image.uid_checksum = data32[3];
	image.signature = data32[4];
	image.cpu = data32[5];
	image.code_checksum = data32[6];
	image.format = data32[7];
	image.major = data32[8] & 0xFF;
	image.minor = data32[8] & 0xFF00;
	image.build = data32[8] >> 16;
	image.timestamp = ((u64)data32[9] << 32) | data32[10];
	image.flags = data32[11];
	image.code_size = data32[12];
	image.data_size = data32[13];
	image.heap_minimum_size = data32[14];
	image.heap_maximum_size = data32[15];
	image.stack_size = data32[16];
	image.BSS_size = data32[17];
	image.entry_point_offset = data32[18];
	image.code_base_address = data32[19];
	image.data_base_address = data32[20];
	image.dll_count = data32[21];
	image.export_offset = data32[22];
	image.export_count = data32[23];
	image.text_size = data32[24];
	image.code_offset = data32[25];
	image.data_offset = data32[26];
	image.import_offset = data32[27];
	image.code_relocation_offset = data32[28];
	image.data_relocation_offset = data32[29];
	image.priority = data32[30];

	// Get the flags
	image.executable_type = (image.flags >> 0) & 0x1;
	image.call_entry_point = (image.flags >> 1) & 0x1;
	image.fixed_address = (image.flags >> 2) & 0x1;
	image.abi = (image.flags >> 3) & 0x3;
	image.entry_point_type = (image.flags >> 5) & 0x7;
	image.header_format = (image.flags >> 24) & 0xF;
	image.import_format = (image.flags >> 28) & 0xF;

	// The entry point address
	image.entry_point = image.code_offset;

	// Check the validity and support for stuff
	// TODO: DLL loading support
	if (!image.executable_type)
	{
		log(ERROR, "DLLs aren't supported");
		return false;
	}

	// TODO: What if there's no entry point?
	if (!image.call_entry_point)
	{
		log(ERROR, "Entry point call unspecified. What do we do?");
		return false;
	}

	// TODO: Support relocations
	// TODO: Is this check even proper? No idea.
	if (image.fixed_address)
	{
		log(ERROR, "Non-fixed address executables aren't supported.");
		return false;
	}

	// TODO: What the hell is an EKA2 entry point?
	if (image.entry_point_type != 0)
	{
		log(ERROR, "EKA2 entry point type is unsupported.");
		return false;
	}

	// TODO: Other import format support
	if (image.import_format != 0)
	{
		log(ERROR, "Other import formats not implemented.");
		return false;
	}

	// TODO: EABI support
	if (image.abi != 0)
	{
		log(ERROR, "EABI ABI is unsupported.");
		return false;
	}

	// Check the signature
	if (image.signature != 'COPE') // 'EPOC' backwards for little-endian
	{
		log(ERROR, "Invalid E32Image signature.");
		return false;
	}

	// Check the UID checksum validity
	u8 uids[12] = {};
	u8_from_32(uids, image.uid1);
	u8_from_32(uids + 4, image.uid2);
	u8_from_32(uids + 8, image.uid3);

	u8 even_bytes[] = { uids[0], uids[2], uids[4], uids[6], uids[8], uids[10] };
	u8 odd_bytes[] = { uids[1], uids[3], uids[5], uids[7], uids[9], uids[11] };

	u32 uid_crc = ((u32)crc16_ccitt(odd_bytes) << 16) | crc16_ccitt(even_bytes);

	if (uid_crc != image.uid_checksum)
	{
		log(ERROR, "UID checksum mismatch.");
		return false;
	}

	return true;
}