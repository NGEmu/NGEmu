#include "stdafx.h"
#include "Modules.h"

extern HLE::Module EUSER;

std::vector<ModuleInfo> module_list =
{
	{ 0x0, "EUSER", &EUSER },
	//{ 0x1, "BAFL", nullptr },
};

void HLE::initialize()
{
	for (auto& module : module_list)
	{
		if (module.module)
		{
			module.module->functions.resize(4096, HLE::ModuleFunction(0, nullptr, nullptr, nullptr));
			module.module->initialize();
		}
	}
}

u32 HLE::register_function(HLE::ModuleFunction function)
{
	function.module->functions.emplace(function.module->functions.begin() + function.id, std::move(function));
	return function.id - 1;
}

void HLE::call_HLE(CPU& cpu, u8 module, u32 ordinal)
{
	if (module > module_list.size())
	{
		return;
	}

	if (auto function = &module_list[module].module->functions[ordinal])
	{
		function->function(cpu);
	}
	else
	{
		log(DEBUG, "Unknown function: %d from %s", ordinal, module_list[module].name);
	}
}