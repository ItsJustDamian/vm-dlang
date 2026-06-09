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
				break;
			case dlang::DlangType::Array:
				printf("%s", obj.arrayToString().c_str());
				break;
			case dlang::DlangType::Map:
				printf("%s", obj.mapToString().c_str());
				break;
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
	
		vm->registerNativeFunction("sizeof", [](vm::DLangVirtualMachine* vm) -> bool {
			auto typeObj = vm->pop();
			int size = 0;
			switch (typeObj.type)
			{
			case DlangType::Integer:
				size = sizeof(int);
				break;
			case DlangType::Float:
				size = sizeof(float);
				break;
			case DlangType::String:
				size = vm->getStringFromPool(typeObj.intValue).size();
				break;
			case DlangType::Boolean:
				size = sizeof(bool);
				break;
			case DlangType::Pointer:
				size = sizeof(void*);
				break;
			case DlangType::Array:
				size = typeObj.arrayElements->size();
				break;
			case DlangType::Map:
				size = typeObj.mapElements->size();
				break;

			default:
				throw std::runtime_error("Unsupported type for sizeof function.");
			}
			vm->push(DlangObject(size));
			return true;
		}, "*", 1);

		vm->registerNativeFunction("substr", [](vm::DLangVirtualMachine* vm) -> bool {

			auto lengthObj = DlangObject(-1);
			if(vm->getStackSize() > 2)
				lengthObj = vm->pop();
			
			auto startObj = vm->pop();
			
			auto strObj = vm->pop();
			auto str = vm->getStringFromPool(strObj.intValue);
			auto newStr = str.substr(startObj.intValue, (lengthObj.intValue == -1 ? str.length() : lengthObj.intValue));

			vm->push(DlangObject(vm->addToStringPool(newStr), dlang::DlangType::String));
			return true;

			}, "str", 1);
	}
}