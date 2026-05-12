#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

namespace dlang
{
	enum class DlangType : unsigned char
	{
		None = 0,
		Integer = 1,
		Float = 2,
		String = 3,
		Boolean = 4,
		DlangFunction = 5,
		ScopeVariable = 6
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
		LESS_THAN = 0x23,
		CALL = 0x30,
	};

	struct DlangObject
	{
		DlangType type;
		union
		{
			int intValue;
			std::string* strValue;
			void* objValue;
			std::vector<void*>* arrValue;
			void* funcValue;
			float floatValue;
			bool boolValue;
		};
		
		DlangObject() : type(DlangType::None), objValue(0) {}
		DlangObject(int value) : type(DlangType::Integer), intValue(value) {}
		DlangObject(float value) : type(DlangType::Float), floatValue(value) {}
		DlangObject(const std::string& value) : type(DlangType::String), strValue(new std::string(value)) {}
		DlangObject(bool value) : type(DlangType::Boolean), boolValue(value) {}
	};

	struct DlangFunction
	{
		std::string name;
		size_t numArgs = 0;
		size_t locationIp = 0;
		bool isNative = false;
		void* nativePtr = nullptr;

		DlangFunction() = default;
		DlangFunction(const std::string& funcName, size_t args, size_t ip) : name(funcName), numArgs(args), locationIp(ip) {}
	};
}