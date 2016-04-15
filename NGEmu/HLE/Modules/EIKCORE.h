#pragma once

#include "EUSER.h"

namespace EIKCORE
{
	class CApaApplication : public EUSER::CBase
	{
	};

	class CEikApplication : public CApaApplication
	{
	protected:
		CEikApplication();
	};

	// Wrapper functions
	void CEikApplication_1(u32* object);
}