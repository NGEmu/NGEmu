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
		return (read8(address + 1) << 8) | read8(address);
	}

	inline u32 read32(u32 address)
	{
		return (read16(address + 2) << 16) | read16(address);
	}

	inline void write8(u32 address, u8 value)
	{
		memory[address] = value;
	}

	inline void write16(u32 address, u16 value)
	{
		write8(address + 1, value >> 8);
		write8(address, value & 0xFF);
	}

	inline void write32(u32 address, u32 value)
	{
		write16(address + 2, value >> 16);
		write16(address, value & 0xFFFF);
	}

private:
	u8 memory[0x1000000] = {};
};

// Bit manipulation functions
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

template<typename T>
inline T reverse(T n, u64 b = std::numeric_limits<T>::digits)
{
	T reversed = 0;

	for (u64 i = 0; i < b; ++i, n >>= 1)
	{
		reversed = (reversed >> 1) | (n & 0x01);
	}

	return reversed;
}

inline u8 rotate_right(u8 value, u8 shift)
{
#ifdef _MSC_VER
	return _rotr8(value, shift);
#else
	#error "Rotate right 8 unimplemented"
#endif
}

inline u16 rotate_right(u16 value, u8 shift)
{
#ifdef _MSC_VER
	return _rotr16(value, shift);
#else
	#error "Rotate right 16 unimplemented"
#endif
}

inline u8 rotate_left(u8 value, u8 shift)
{
#ifdef _MSC_VER
	return _rotl8(value, shift);
#else
	#error "Rotate left 8 unimplemented"
#endif
}

inline u16 rotate_left(u16 value, u8 shift)
{
#ifdef _MSC_VER
	return _rotl16(value, shift);
#else
	#error "Rotate left 16 unimplemented"
#endif
}

// Carry and overflow functions
//inline s32 carry_from(u32 a, u32 b)
//{
//	if ((u64)(a)+b > UINT_MAX)
//	{
//		return 1;
//	}
//	else
//	{
//		return 0;
//	}
//};
//
//inline s32 overflow_from(s32 a, s32 b)
//{
//	s32 c = a + b;
//
//	if ((a > 0 && b > 0 && c < 0) || (a < 0 && b <0 && c >0))
//	{
//		return 1;
//	}
//	else
//	{
//		return 0;
//	}
//};
//
//inline s32 carry_from(u32 a, u32 b, s32 carry)
//{
//	if (carry == 0)
//	{
//		return carry_from(a, b);
//	}
//	else
//	{
//		if (carry_from(a, b + 1) || carry_from(a + 1, b) || carry_from(a, b))
//		{
//			return 1;
//		}
//		else
//		{
//			return 0;
//		}
//	}
//};
//
//inline s32 overflow_from(s32 a, s32 b, s32 carry)
//{
//	if (!get_C())
//	{
//		return overflow_from(a, b);
//	}
//	else
//	{
//		if (overflow_from(a, b + 1) || overflow_from(a + 1, b) || overflow_from(a, b))
//		{
//			return 1;
//		}
//		else
//		{
//			return 0;
//		}
//	}
//};