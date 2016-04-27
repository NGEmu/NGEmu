#pragma once

#include "APPARC.h"

namespace EIKCORE
{
	class CEikApplication : public APPARC::CApaApplication
	{
	protected:
		CEikApplication();
	};

	// Wrapper functions
	void CEikApplication_1(u32* object);
}