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

	inline bool native_messagebox(vm::DLangVirtualMachine* vm)
	{
		auto message = vm->pop();
		auto title = vm->pop();

		MessageBoxA(NULL, (*message.strValue).c_str(), (*title.strValue).c_str(), NULL);

		return true;
	}

	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("print", reinterpret_cast<void*>(&print_native), "*", 1);
		vm->registerNativeFunction("messagebox", reinterpret_cast<void*>(&native_messagebox), "*", 2);

		/*vm->registerNativeFunction("abs", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(abs(value.intValue)));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(abs(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for abs function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("sqrt", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(sqrt(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(sqrt(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for sqrt function. Expected Integer or Float.");
			return true;
		}, "math", 1);

		vm->registerNativeFunction("pow", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto exponent = vm->pop();
			auto base = vm->pop();
			if (base.type == dlang::DlangType::Integer && exponent.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(pow(base.intValue, exponent.intValue))));
			else if (base.type == dlang::DlangType::Float && exponent.type == dlang::DlangType::Float)
				vm->push(DlangObject(pow(base.floatValue, exponent.floatValue)));
			else
				throw std::runtime_error("Invalid argument types for pow function. Expected Integer or Float.");
			return true;
			}, "math", 2);

		vm->registerNativeFunction("exp", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(exp(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(exp(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for exp function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("log", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(log(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(log(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for log function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("round", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(round(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(round(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for round function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("floor", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(floor(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(floor(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for floor function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("ceil", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(ceil(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(ceil(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for ceil function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("sin", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(sin(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(sin(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for sin function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("cos", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(cos(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(cos(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for cos function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("tan", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(tan(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(tan(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for tan function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("asin", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(asin(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(asin(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for asin function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("acos", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(acos(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(acos(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for acos function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("atan", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto value = vm->pop();
			if (value.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(atan(value.intValue))));
			else if (value.type == dlang::DlangType::Float)
				vm->push(DlangObject(atan(value.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for atan function. Expected Integer or Float.");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("atan2", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto y = vm->pop();
			auto x = vm->pop();
			if (x.type == dlang::DlangType::Integer && y.type == dlang::DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(atan2(y.intValue, x.intValue))));
			else if (x.type == dlang::DlangType::Float && y.type == dlang::DlangType::Float)
				vm->push(DlangObject(atan2(y.floatValue, x.floatValue)));
			else
				throw std::runtime_error("Invalid argument types for atan2 function. Expected Integer or Float.");
			return true;
			}, "math", 2);

		vm->registerNativeFunction("random", &[](vm::DLangVirtualMachine* vm) -> bool {
			auto maxValue = vm->pop();
			if (maxValue.type == dlang::DlangType::Integer)
				vm->push(DlangObject(rand() % maxValue.intValue));
			else if (maxValue.type == dlang::DlangType::Float)
				vm->push(DlangObject(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * maxValue.floatValue));
			else
				throw std::runtime_error("Invalid argument type for random function. Expected Integer or Float.");
			return true;
			}, "math", 1);*/
	}
}