#pragma once

using function_caller = void(*)(CPU&);

namespace HLE_function
{
	template<typename T>
	struct cast_register
	{
		inline static T from_register(CPU& cpu, const u32 reg)
		{
			return static_cast<T>(cast_register<std::underlying_type<T>>::from_register(cpu, reg));
		}
	};

	template<>
	struct cast_register<u8*>
	{
		inline static u8* from_register(CPU& cpu, const u32 reg)
		{
			return reinterpret_cast<u8*>(&cpu.memory.memory[reg]);
		}
	};

	template<typename T>
	inline T cast_from_register(CPU& cpu, const u32 reg)
	{
		return cast_register<T>::from_register(cpu, reg);
	}

	template<typename T, bool type>
	struct bind_argument
	{
		static inline T get_arg(CPU& cpu)
		{
			if (!type)
			{
				return cast_from_register<T>(cpu, cpu.GPR[0]);
			}
			else
			{
				static_assert(true, "Arguments from stack not supported");
			}
		}
	};

	template<typename RT, typename... Args>
	inline RT call(CPU& cpu, RT(*function)(Args...))
	{
		return function(bind_argument<Args, static_cast<bool>(sizeof...(Args) > 4)>::get_arg(cpu)...);
	}

	template<typename T, typename... Types, typename RT, typename... Args>
	inline RT call(CPU& cpu, RT(*function)(Args...))
	{
		return call<Types...>(cpu, function);
	}

	template<typename RT, typename... T>
	struct function_binder;

	template<typename... T>
	struct function_binder<void, T...>
	{
		using function = void(*)(T...);

		static inline void do_call(CPU& cpu, function function)
		{
			call<T...>(cpu, function);
		}
	};

	template<typename RT, typename... T>
	inline void do_call(CPU& cpu, RT(*function)(T...))
	{
		function_binder<RT, T...>::do_call(cpu, function);
	}
}

namespace HLE
{
	class Module;
	class ModuleFunction
	{
	public:
		u32 id;
		Module* module;
		const char* name;
		function_caller function;

		ModuleFunction(u32 id, Module* module, const char* name, function_caller function)
			: id(id)
			, module(module)
			, name(name)
			, function(function)
		{
		}
	};

	class Module
	{
	public:
		Module(const std::string& name, void(*initialize)())
			: name(name)
			, initialize(initialize)
		{
		}

		u8 id;
		std::string name;
		void(*initialize)();
		std::vector<ModuleFunction> functions;
	};

	void initialize();
	void register_function(ModuleFunction function);
	void call_HLE(CPU& cpu, u8 module, u32 ordinal);
}

struct ModuleInfo
{
	const u32 id;
	const char* name;
	HLE::Module* module;
};

extern std::vector<ModuleInfo> module_list;

#define BIND_FUNCTION(function) [](CPU& cpu) { HLE_function::do_call(cpu, function); }
#define REGISTER_HLE(module, function, id) HLE::register_function(HLE::ModuleFunction(id, &module, #function, BIND_FUNCTION(function)))