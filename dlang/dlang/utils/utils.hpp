#pragma once
#include <iostream>
#include "../vm.hpp"

namespace dlang
{
	namespace functions
	{
		namespace utils
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
						printf("%s", (*obj.strValue).c_str());
						break;
					case dlang::DlangType::Float:
						printf("%.001f", obj.floatValue);
						break;
					case dlang::DlangType::Boolean:
						printf("%s", (obj.boolValue ? "true" : "false"));
						break;
					}
				}

				return true;
			}

			inline void initFunctions(vm::DLangVirtualMachine* vm)
			{
				vm->registerNativeFunction("print", reinterpret_cast<void*>(&print_native), "*", 1);
			}
		}
	}
}