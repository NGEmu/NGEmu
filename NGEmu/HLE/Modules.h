#pragma once

using function_caller = void(*)(CPU&);

namespace HLE_function
{
	template<typename T>
	struct cast_register
	{
		inline static T from_register(CPU& cpu, const u32 reg)
		{
			return static_cast<T>(cast_register<T>::from_register(cpu, reg));
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

	template<>
	struct cast_register<s32>
	{
		inline static s32 from_register(CPU& cpu, const u32 reg)
		{
			return static_cast<s32>(reg);
		}
	};

	template<typename T>
	inline T cast_from_register(CPU& cpu, const u32 reg)
	{
		return cast_register<T>::from_register(cpu, reg);
	}

	template<typename T, bool stack>
	struct bind_argument
	{
		static inline T get_arg(CPU& cpu, u8 index)
		{
			if (!stack)
			{
				return cast_from_register<T>(cpu, cpu.GPR[index]);
			}
			else
			{
				static_assert(true, "Arguments from stack not supported");
			}
		}
	};

	template<typename RT, typename Arg0, typename... Args, u64... Is>
	inline RT call(CPU& cpu, RT(*function)(Arg0, Args...), std::index_sequence<Is...>)
	{
		constexpr u16 arg_count = sizeof...(Args);
		constexpr bool use_stack = static_cast<bool>(arg_count > 3);
		return function(reinterpret_cast<Arg0>(&cpu.memory.memory[cpu.SP]), bind_argument<Args, use_stack>::get_arg(cpu, Is)...);
	}

	template<typename T, typename... Types, typename RT, typename... Args>
	inline RT call(CPU& cpu, RT(*function)(Args...))
	{
		return call(cpu, function, std::make_index_sequence<sizeof...(Args) - 1>{});
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
	u32 register_function(ModuleFunction function);
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