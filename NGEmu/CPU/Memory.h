#pragma once

class Memory
{
public:
	inline u8 read8(u32 address)
	{
		return memory[address];
	}

	inline u16 read16(u32 address)
	{
		return (read8(address) << 8) | read8(address + 1);
	}

	inline u32 read32(u32 address)
	{
		return (read16(address) << 16) | read16(address + 2);
	}

	void write8(u32 address, u8 data)
	{
		memory[address] = data;
	}

private:
	u8 memory[0x380000] = {};
};

// Memory manipulation functions
inline bool get_bit(u32& value, u8 bit)
{
	return (value >> bit) & 1;
}

inline void set_bit(u32& value, u8 bit)
{
	value |= 1 << bit;
}

inline void clear_bit(u32& value, u8 bit)
{
	value &= ~(1 << bit);
}

inline void set_bit(u32& value, u8 bit, bool state)
{
	if (state)
	{
		set_bit(value, bit);
	}
	else
	{
		clear_bit(value, bit);
	}
}