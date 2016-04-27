#include "stdafx.h"
#include "APPARC.h"
#include "HLE/Modules.h"

extern HLE::Module mAPPARC;
using namespace APPARC;

// CApaApplication
CApaApplication::CApaApplication()
{
}

// Wrapper functions
namespace APPARC
{
}

HLE::Module mAPPARC("EIKCORE", []()
{
});