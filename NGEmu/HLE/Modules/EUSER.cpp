#include "stdafx.h"
#include "EUSER.h"
#include "HLE/Modules.h"

extern HLE::Module mEUSER;
using namespace EUSER;

// Set of static functions
s32 User::StringLength(u16* aString)
{
	u16 *string_pointer = aString;

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
void TBufBase16_1(u32* object, s32 aMaxLength)
{
	new((TBufBase16*)object)TBufBase16(aMaxLength);
}

void TBufBase16_3(u32* object, u16* aString, s32 aMaxLength)
{
	new((TBufBase16*)object)TBufBase16(aString, aMaxLength);
}

HLE::Module mEUSER("EUSER", []()
{
	REGISTER_HLE(mEUSER, TBufBase16_1, 1283);
});