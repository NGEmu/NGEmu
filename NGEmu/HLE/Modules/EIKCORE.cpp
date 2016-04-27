#include "stdafx.h"
#include "EIKCORE.h"
#include "HLE/Modules.h"

extern HLE::Module mEIKCORE;
using namespace EIKCORE;

// CEikApplication
CEikApplication::CEikApplication()
{
}

// Wrapper functions
namespace EIKCORE
{
	void CEikApplication_1(u32* object)
	{
		log(WARNING, "CEikApplication::CEikApplication()");
		//new((CEikApplication*)object)CEikApplication();
	}
}

HLE::Module mEIKCORE("EIKCORE", []()
{
	REGISTER_HLE(mEIKCORE, EIKCORE::CEikApplication_1, 222);
});