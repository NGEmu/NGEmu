#pragma once

namespace HLE
{
	class Module
	{
	public:
		Module(const std::string& name, void(*init)())
		{
			initialize = init;
		}

		u8 id;
		void(*initialize)();
		std::map<u16, void*> functions;
	};

	class ModuleFunction
	{
	public:
		u32 id;
		Module* module;
		const char* name;
		void* function;

		ModuleFunction(u32 id, Module* module, const char* name, void* function)
			: id(id)
			, module(module)
			, name(name)
			, function(function)
		{
		}
	};

	// Template for calling functions
	/*template<typename T, T function, typename... Args, typename RT = std::result_of_t<T(Args...)>> inline RT callFunc(Args&&... args)
	{
		log(DEBUG, "Calling");
	}*/

	void initialize();
	void register_function(ModuleFunction function);
	void call_HLE(u8 module, u32 ordinal);
}

struct ModuleInfo
{
	const u32 id;
	const char* name;
	const HLE::Module* module;
};

extern std::vector<ModuleInfo> module_list;

#define REGISTER_HLE(module, function, id) HLE::register_function(HLE::ModuleFunction(id, &module, #function, &function))