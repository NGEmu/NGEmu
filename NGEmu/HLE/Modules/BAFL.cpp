#include "stdafx.h"
#include "BAFL.h"
#include "HLE/Modules.h"

extern HLE::Module mBAFL;
using namespace BAFL;

// Command line argument functions
u32 CCommandLineArguments::NewL()
{
	log(ERROR, "TODO: CCommandLineArguments::NewL()");
	return 0;
}

// Wrapper functions
namespace BAFL
{
	u32 CCommandLineArguments_NewL()
	{
		log(WARNING, "CCommandLineArguments::NewL()");
		return CCommandLineArguments::NewL();
	}
}

HLE::Module mBAFL("BAFL", []()
{
	REGISTER_HLE(mBAFL, BAFL::CCommandLineArguments_NewL, 88);
});