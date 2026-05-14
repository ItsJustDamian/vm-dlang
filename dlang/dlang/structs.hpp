#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

typedef unsigned __int64 DWORD64, * PDWORD64;

namespace dlang
{
	namespace vm { class DLangVirtualMachine; }

	using NativeCallback = bool(*)(vm::DLangVirtualMachine*);

	enum class DlangType : unsigned char
	{
		None = 0,
		Integer = 1,
		Float = 2,
		String = 3,
		Boolean = 4,
		DlangFunction = 5,
		ScopeVariable = 6,
		Pointer = 7
	};

	enum Opcode : uint8_t
	{
		NONE = 0x0,
		PUSH_INT = 0x1,
		PUSH_STRING = 0x2,
		PUSH_BOOL = 0x3,
		PUSH_FLOAT = 0x4,
		POP = 0x6,
		ADD = 0x7,
		SUB = 0x8,
		DEC = 0x9,
		MP = 0xA,
		DIV = 0xB,
		EOC = 0x10,
		STORE_VAR = 0x12,
		LOAD_VAR = 0x13,
		JUMP = 0x20,
		COMPARE = 0x21,
		JUMP_IF_FALSE = 0x22,
		CALL = 0x30,
		LESS_THAN = 0x23,
		GREATER_THAN = 0x24,
		LESS_OR_EQUAL_THAN = 0x25,
		GREATER_OR_EQUAL_THAN = 0x26,
		NOT_EQUALS_TO = 0x27,
		DEF_FUNC = 0x40,
		END_FUNC = 0x41,
		RETURN = 0x42
	};

	struct DlangObject
	{
		DlangType type;
		union
		{
			void* objPtr;
			int intValue;
			DWORD64 ptrValue;
		};
		
		DlangObject() : type(DlangType::None), ptrValue(0) {}
		DlangObject(int value, DlangType _type = DlangType::Integer) : type(_type), intValue(value) {}
		DlangObject(DWORD64 value) : type(DlangType::Pointer) { ptrValue = value; }
	};

	struct DlangFunction
	{
		std::string name;
		size_t numArgs = 0;
		size_t startInstruction = 0;
		size_t bodySize = 0;
		bool isNative = false;
		NativeCallback nativePtr = nullptr;
		std::vector<std::string> argNames;

		DlangFunction() = default;
		DlangFunction(const std::string& funcName, size_t args, size_t ip) : name(funcName), numArgs(args), startInstruction(ip) {}
	};
}