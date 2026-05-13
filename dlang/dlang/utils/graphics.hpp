#pragma once
#include <iostream>
#include <Windows.h>
#include "../vm.hpp"

namespace dlang::functions::graphics
{
	inline bool native_CreateWindow(vm::DLangVirtualMachine* vm)
	{
		MessageBoxA(NULL, "This func works", "lol.", NULL);
		return true;
	}

	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("CreateWindow", reinterpret_cast<void*>(&native_CreateWindow), "gfx", 0);
	}
}