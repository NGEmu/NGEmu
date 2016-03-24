#include "stdafx.h"
#include "Modules.h"

extern HLE::Module EUSER;

std::vector<ModuleInfo> module_list =
{
	{ 0x0, "EUSER", &EUSER },
	//{ 0x2, "BAFL", nullptr },
};

void HLE::initialize()
{
	for (auto& module : module_list)
	{
		if (module.module)
		{
			module.module->functions.resize(4096);
			module.module->initialize();
		}
	}
}

void HLE::register_function(HLE::ModuleFunction function)
{
	log(DEBUG, "Registering: %s", function.name);
	function.module->functions.emplace(function.module->functions.begin() + function.id, std::move(function));
}

void HLE::call_HLE(CPU& cpu, u8 module, u32 ordinal)
{
	if (module > module_list.size())
	{
		return;
	}

	log(DEBUG, "module: %d", module);
	log(DEBUG, "ordinal: %d", ordinal);

	if (auto function = &module_list[module].module->functions[ordinal])
	{
		log(DEBUG, "Found function");
		function->function(cpu);
	}
}