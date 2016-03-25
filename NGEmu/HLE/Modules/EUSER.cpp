#include "stdafx.h"
#include "EUSER.h"
#include "HLE/Modules.h"

extern HLE::Module EUSER;

void BufBase16(u32* stack, u8* descriptor, s32 some_int)
{
	log(DEBUG, "TBufBase16 init! Success");
	log(DEBUG, "stack: %p", stack);
	log(DEBUG, "descriptor: %p", descriptor);
	log(DEBUG, "some_int: 0x%X", some_int);
	*descriptor = 0xFF;
}

HLE::Module EUSER("EUSER", []()
{
	REGISTER_HLE(EUSER, BufBase16, 1283);
});