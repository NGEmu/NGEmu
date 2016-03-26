#include "stdafx.h"
#include "BAFL.h"
#include "HLE/Modules.h"

extern HLE::Module mBAFL;
using namespace BAFL;

// Command line argument functions
CCommandLineArguments* CCommandLineArguments::NewL()
{
	log(ERROR, "TODO: CCommandLineArguments::NewL()");
	return nullptr;
}

// Wrapper functions
u64* CCommandLineArguments_NewL()
{
	return (u64*)CCommandLineArguments::NewL();
}

HLE::Module mBAFL("BAFL", []()
{
	REGISTER_HLE(mBAFL, CCommandLineArguments_NewL, 88);
});