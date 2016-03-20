#include "stdafx.h"
#include "Modules.h"

extern HLE::Module EUSER;

std::vector<ModuleInfo> module_list =
{
	{ 0x1, "EUSER", &EUSER },
	//{ 0x2, "BAFL", nullptr },
};

void HLE::initialize()
{
	for (auto& module : module_list)
	{
		module.module->initialize();
	}
}

void HLE::register_function(HLE::ModuleFunction function)
{
	log(DEBUG, "Registering: %s", function.name);
	function.module->functions.emplace(function.id, function.function);
}

void HLE::call_HLE(u8 module, u32 ordinal)
{
	log(DEBUG, "HLE CALL");
	log(DEBUG, "module: %d", module);
	log(DEBUG, "ordinal: %d", ordinal);
}