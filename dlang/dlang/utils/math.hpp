#pragma once
#include <iostream>
#include <Windows.h>
#include "../vm.hpp"

namespace dlang::functions::math
{
	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("random_seed", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				srand(time(0));
			else {
				auto seedObj = vm->pop();
				if (seedObj.type == DlangType::Integer)
					srand(seedObj.intValue);
				else
					throw std::runtime_error("Invalid argument type for math.random_seed, expected (int seed or empty)");
			}
			
			return true;
			}, "math", 0);

		vm->registerNativeFunction("random", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::Integer }))
				throw std::runtime_error("Invalid arguments for math.random, expected (int max) or (int min, int max)");
			
			int min = 0;
			int max = 10;

			if (vm->getStackSize() >= 2)
			{
				max = vm->pop().intValue;
				min = vm->pop().intValue;
			}
			else {
				max = vm->pop().intValue;
			}

			auto randomValue = min + rand() % (max - min + 1);
			vm->push(DlangObject(randomValue));

			return true;
			}, "math", 1);

		vm->registerNativeFunction("abs", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.abs, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(abs(valueObj.intValue)));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(fabs(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.abs, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("sqrt", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.sqrt, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(sqrt(valueObj.intValue))));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(sqrt(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.sqrt, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("pow", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() < 2)
				throw std::runtime_error("Not enough arguments for math.pow, expected (int base, int exponent) or (float base, float exponent)");
			auto exponentObj = vm->pop();
			auto baseObj = vm->pop();
			if (baseObj.type == DlangType::Integer && exponentObj.type == DlangType::Integer)
				vm->push(DlangObject(static_cast<int>(pow(baseObj.intValue, exponentObj.intValue))));
			else if ((baseObj.type == DlangType::Integer || baseObj.type == DlangType::Float) && (exponentObj.type == DlangType::Integer || exponentObj.type == DlangType::Float))
			{
				float base = (baseObj.type == DlangType::Float) ? baseObj.floatValue : static_cast<float>(baseObj.intValue);
				float exponent = (exponentObj.type == DlangType::Float) ? exponentObj.floatValue : static_cast<float>(exponentObj.intValue);
				vm->push(DlangObject(pow(base, exponent)));
			}
			else
				throw std::runtime_error("Invalid argument types for math.pow, expected (int base, int exponent) or (float base, float exponent)");
			return true;
			}, "math", 2);

		vm->registerNativeFunction("floor", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.floor, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(valueObj.intValue));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(floor(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.floor, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("ceil", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.ceil, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(valueObj.intValue));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(ceil(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.ceil, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("round", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.round, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(valueObj.intValue));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(round(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.round, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("sin", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.sin, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(sin(static_cast<float>(valueObj.intValue))));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(sin(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.sin, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("cos", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.cos, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(cos(static_cast<float>(valueObj.intValue))));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(cos(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.cos, expected (int value) or (float value)");
			return true;
			}, "math", 1);

		vm->registerNativeFunction("tan", [](vm::DLangVirtualMachine* vm) -> bool {
			if (vm->getStackSize() <= 0)
				throw std::runtime_error("Not enough arguments for math.tan, expected (int value) or (float value)");
			auto valueObj = vm->pop();
			if (valueObj.type == DlangType::Integer)
				vm->push(DlangObject(tan(static_cast<float>(valueObj.intValue))));
			else if (valueObj.type == DlangType::Float)
				vm->push(DlangObject(tan(valueObj.floatValue)));
			else
				throw std::runtime_error("Invalid argument type for math.tan, expected (int value) or (float value)");
			return true;
			}, "math", 1);
	}
}