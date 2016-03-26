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
	image.data_checksum = data32[7];
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
	image.executable_type = (image.flags >> 0) & 1;
	image.call_entry_point = (image.flags >> 1) & 1;
	image.fixed_address = (image.flags >> 2) & 1;
	image.abi = (image.flags >> 3) & 0x3;
	image.entry_point_type = (image.flags >> 5) & 7;
	image.header_format = (image.flags >> 24) & 0xF;
	image.import_format = (image.flags >> 28) & 0xF;

	// The entry point address
	image.entry_point = image.code_offset;

	// Check the validity and support for stuff
	// TODO: Non-fixed address executables support
	if (image.fixed_address)
	{
		log(ERROR, "Non-fixed address executables aren't supported.");
		return false;
	}

	// J-format with compression and V-format isn't supported (some newer games have Symbian OS 9 headers)
	if (image.header_format != 0 && image.data_checksum != 0)
	{
		log(ERROR, "Only basic header type is supported.");
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

	// TODO: Data relocation support
	if (image.data_relocation_offset != 0)
	{
		log(ERROR, "Data relocation not supported");
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

	// Calculate import count
	u32* imports = reinterpret_cast<u32*>(image.data.data() + image.code_offset + image.text_size);
	image.import_count = 0;

	while (*imports++)
	{
		image.import_count++;
	}
	
	if (image.dll_count == 0)
	{
		log(ERROR, "DLL count is 0, please report this. (%d)", image.import_count);
		return false;
	}

	// Perform relocations
	if (image.code_relocation_offset != 0)
	{
		relocate(image, image.data.data() + image.code_relocation_offset, image.data.data() + image.code_offset, image.code_base_address);
	}

	// The number of exports should always be at least 1
	if (image.export_count != 1)
	{
		log(ERROR, "Export count is not 1. Report this. (%d)", image.export_count);
		return false;
	}

	// There is always one exported function called NewApplication(), which starts the execution of the application. It's called through apprun.exe on pre-Symbian OS 9.
	image.entry_point = data32[image.export_offset / 4];

	// Patch imports
	patch_imports(image);

	// Don't include the header in the data
	std::vector<decltype(image.data)::value_type>(image.data.begin() + image.code_offset, image.data.end()).swap(image.data);

	return true;
}

// Relocates a section. Data is the relocation information and destination the relocation destination. Delta is the linked address - actual address.
void loader::relocate(E32Image& image, u8* data, u8* destination, u32 delta)
{
	data += 8;
	s32 relocation_count = *(s32*)(data - 4);
	u32 page = 0;
	s32 size = 0;
	u32 code_start = image.code_offset;
	u32 code_end = code_start + image.code_size;
	u32 iat_start = code_start + image.text_size;
	u32 iat_end = iat_start + (image.import_count * sizeof(u32));

	// TODO: Support more types of relocations
	while (relocation_count > 0)
	{
		if (size > 0)
		{
			u16 offset = *(u16*)data;
			data += 2;

			if (offset != 0)
			{
				u32 virtual_address = page + (offset & 0x0FFF);
				u32* data_pointer = reinterpret_cast<u32*>(destination + virtual_address);
				u32 data = *data_pointer;

				if (data >= iat_start && data < iat_end)
				{
					log(ERROR, "IAT relocation (TODO)");
					*data_pointer = data;
				}
				else if (data >= code_start && data < code_end)
				{
					log(ERROR, "Code relocation (TODO)");
					*data_pointer = data;
				}
				else
				{
					*data_pointer = data - delta;
				}

				relocation_count--;
			}

			size -= 2;
		}
		else
		{
			page = *(u32*)data;
			size = *(u32*)(data + 4);
			data += 8;
			size -= 8;
		}
	}
}

// Patches the imports with the module ID
void loader::patch_imports(E32Image& image)
{
	u8* data = image.data.data() + image.import_offset;
	s32 size = *(s32*)data;
	s32 total_import_count = 0;

	// HACK: There are two import tables, first one (the "real" one) containing everything we need, but is unutilized by the application.
	//       The second one is utilized but is missing the headers. We get headers from first one and patch imports in the second one.
	for (s32 i = 0; i < image.dll_count; i++)
	{
		data = image.data.data() + image.import_offset + (i * 8) + (total_import_count * sizeof(u32)) + 4;

		u32 dll_name_offset = *(u32*)data;
		s32 import_count = *(s32*)(data + 4);

		data = image.data.data() + image.code_offset + image.text_size + (total_import_count * sizeof(u32));
		total_import_count += import_count;

		std::string name((char*)(image.data.data() + image.import_offset + dll_name_offset));
		name = name.substr(0, name.find('['));

		u32* import = (u32*)data;

		// Determine the module ID
		u8 module_id = 0xFF;

		for (auto& module : module_list)
		{
			if (module.name == name)
			{
				module_id = module.id;
				break;
			}
		}

		// No module was found with such a name
		if (module_id == 0xFF)
		{
			log(WARNING, "Unknown module: %s", name.c_str());
		}

		// Insert module IDs
		while (import_count--)
		{
			u32 id = *import | (u32)module_id << 24;
			*import = id;
			import++;
		}
	}
}