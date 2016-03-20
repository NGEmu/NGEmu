#include "stdafx.h"
#include "EUSER.h"
#include "HLE/Modules.h"

extern HLE::Module EUSER;

void BufBase16(const u8* desc, s32 some_int)
{
	log(DEBUG, "TBufBase16 init! Success");
}

HLE::Module EUSER("EUSER", []()
{
	REGISTER_HLE(EUSER, BufBase16, 1283);
});