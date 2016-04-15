#include "stdafx.h"
#include "EUSER.h"
#include "HLE/Modules.h"

extern HLE::Module mEUSER;
using namespace EUSER;

// General static functions
void Mem::FillZ(u32* aTrg, s32 aLength)
{
	memset(aTrg, 0, aLength);
}

void User::Leave()
{
	emulator.emulating = false;
}

u32 User::Alloc(s32 aSize)
{
	return emulator.cpu->memory.allocate_heap(aSize);
}

u32 User::AllocZ(s32 aSize)
{
	u32 address = Alloc(aSize);

	if (address)
	{
		Mem::FillZ(emulator.cpu->memory.get_pointer(address), aSize);
	}

	return address;
}

u32 User::AllocZL(s32 aSize)
{
	u32 address = AllocL(aSize);
	Mem::FillZ(emulator.cpu->memory.get_pointer(address), aSize);
	return address;
}

u32 User::AllocL(s32 aSize)
{
	u32 address = Alloc(aSize);
	
	if (!address)
	{
		log(ERROR, "AllocL(): Failed to allocate on the heap! Aborting.");
		User::Leave();
	}

	return address;
}

s32 User::StringLength(u16* aString)
{
	u16* string_pointer = aString;

	while (*string_pointer)
	{
		string_pointer++;
	}

	return static_cast<s32>(string_pointer - aString);
}

// Descriptors
// TDesC16
TDesC16::TDesC16(s32 aType, s32 aLength)
{
	iLength = aLength;
	iType = aType;
}

u16* TDesC16::Ptr() const
{
	u16* pointer = nullptr;
	
	switch (Type())
	{
	case EBuf:
	{
		pointer = &((SBuf16*)this)->buf[0];
	}
	break;

	default:
		log(ERROR, "Unknown descriptor type: %d", Type());
	}

	return pointer;
}

inline s32 TDesC16::Type() const
{
	return iType;
}

inline void TDesC16::DoSetLength(s32 aLength)
{
	iLength = aLength;
}

// TDes16
TDes16::TDes16(s32 aType, s32 aLength, s32 aMaxLength) : TDesC16(aType, aLength)
{
	iMaxLength = aMaxLength;
}

void TDes16::SetLength(s32 aLength)
{
	DoSetLength(aLength);

	if (Type() == EBufCPtr)
	{
		log(ERROR, "SetLength(): EBufCPtr not supported");
	}
}

void TDes16::Copy(u16* aString)
{
	s32 length = User::StringLength(aString);
	SetLength(length);
	memcpy(WPtr(), aString, length);
}

inline u16* TDes16::WPtr() const
{
	return Ptr();
}

// TBufBase16
TBufBase16::TBufBase16(s32 aMaxLength) : TDes16(EBuf, 0, aMaxLength)
{
}

TBufBase16::TBufBase16(u16* aString, s32 aMaxLength) : TDes16(EBuf, 0, aMaxLength)
{
	Copy(aString);
}

// Wrapper functions
namespace EUSER
{
	u32 malloc(s32 aSize)
	{
		log(WARNING, "malloc(aSize=0x%X)", aSize);
		return User::Alloc(aSize);
	}

	void TBufBase16_1(u32* object, s32 aMaxLength)
	{
		log(WARNING, "TBufBase16(aMaxLength=0x%X)", aMaxLength);
		new((TBufBase16*)object)TBufBase16(aMaxLength);
	}

	void TBufBase16_3(u32* object, u16* aString, s32 aMaxLength)
	{
		log(WARNING, "TBufBase16(aString=*0x%x, aMaxLength=0x%X)", aString, aMaxLength);
		new((TBufBase16*)object)TBufBase16(aString, aMaxLength);
	}

	u32 CBase_new(u32 aSize)
	{
		log(WARNING, "CBase::new(aSize=0x%X)", aSize);
		return User::AllocZ(aSize);
	}

	u32 CBase_newL(u32 aSize)
	{
		log(WARNING, "CBase::newL(aSize=0x%X)", aSize);
		return User::AllocZL(aSize);
	}
}

HLE::Module mEUSER("EUSER", []()
{
	REGISTER_HLE(mEUSER, EUSER::CBase_newL, 3);
	REGISTER_HLE(mEUSER, EUSER::CBase_new, 1582);
});