#pragma once
#include <iostream>
#include <Windows.h>
#include "../vm.hpp"

namespace dlang::functions::utils
{
	inline bool print_native(vm::DLangVirtualMachine* vm)
	{
		while (vm->getStackSize() > 0)
		{
			auto obj = vm->pop();
			switch (obj.type)
			{
			case dlang::DlangType::Integer:
				printf("%i", obj.intValue);
				break;
			case dlang::DlangType::String:
				printf("%s", vm->getStringFromPool(obj.intValue).c_str());
				break;
			case dlang::DlangType::Float:
				printf("%f", obj.floatValue);
			}
		}

		return true;
	}

	inline bool native_messagebox(vm::DLangVirtualMachine* vm)
	{
		auto message = vm->pop();
		auto title = vm->pop();

		MessageBoxA(NULL, vm->getStringFromPool(message.intValue).c_str(), vm->getStringFromPool(title.intValue).c_str(), NULL);

		return true;
	}

	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("print", print_native, "*", 1);
		vm->registerNativeFunction("messagebox", native_messagebox, "*", 2);
	}
}