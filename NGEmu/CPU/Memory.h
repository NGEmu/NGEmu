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

	u8 memory[0x1000000] = {};
	u32* base_address = reinterpret_cast<u32*>(&memory[0]);

	// So we can use standard libary functions that need pointers in HLE implementation
	inline u32* get_pointer(u32 address)
	{
		return base_address + address;
	}

	// Heap
	std::map<u32, u32> heap;
	u32 heap_begin;
	u32 heap_size;
	u32 heap_cursor;

	inline u32 allocate_heap(u32 size)
	{
		if (size > heap_size)
		{
			log(ERROR, "Size bigger than heap size! Send help.");
			return 0;
		}

		if (heap_cursor > 0x800000)
		{
			log(ERROR, "Heap overflow!");
			return 0;
		}

		u32 alloc_address = 0;
		u32 current_size = 0;
		u32 prev_address_end = 0x100000;

		for (auto& iterator : heap)
		{
			// TODO: Improve allocation logic (allocating before a block with no block before)
			if (prev_address_end < iterator.first) // Allocate inbetween allocations
			{
				log(ERROR, "TODO: Allocation between heaps.");
				alloc_address = prev_address_end + 4;
			}

			prev_address_end = iterator.first + iterator.second;
		}

		if (alloc_address == 0)
		{
			alloc_address = heap_cursor;
		}

		// Increase heap size
		if (alloc_address + size > heap_size)
		{
			heap_size += size + 0x100; // Increase by desired size and a little extra
		}

		heap.emplace(alloc_address, size);
		heap_cursor = alloc_address + size + 4;

		return alloc_address;
	}
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

inline u8 rotate_right(u8& value, u8 shift)
{
#ifdef _MSC_VER
	return _rotr8(value, shift);
#else
	#error "Rotate right 8 unimplemented"
#endif
}

inline u16 rotate_right(u16& value, u8 shift)
{
#ifdef _MSC_VER
	return _rotr16(value, shift);
#else
	#error "Rotate right 16 unimplemented"
#endif
}

inline u8 rotate_left(u8& value, u8 shift)
{
#ifdef _MSC_VER
	return _rotl8(value, shift);
#else
	#error "Rotate left 8 unimplemented"
#endif
}

inline u16 rotate_left(u16& value, u8 shift)
{
#ifdef _MSC_VER
	return _rotl16(value, shift);
#else
	#error "Rotate left 16 unimplemented"
#endif
}

// Borrow, carry and overflow functions
template<typename A, typename B>
inline bool borrow(A& a, B& b)
{
	log(ERROR, "TODO: Borrow detection");
	return a > b;
}

template<typename A, typename B>
inline bool carry(A& a, B& b)
{
	return !borrow(a, b);
}

template<typename A, typename B>
inline bool overflow(A& a, B& b)
{
	log(ERROR, "TODO: Overflow detection");
	return false;
}