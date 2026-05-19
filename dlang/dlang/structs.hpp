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
		Pointer = 7,
		Array = 8,
		Map = 9
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
		LOGIC_AND = 0x50,
		LOGIC_OR = 0x51,

		DEF_FUNC = 0x40,
		END_FUNC = 0x41,
		RETURN = 0x42,

		NEW_ARRAY = 0x60,
		STORE_ARRAY = 0x61,
		LOAD_ARRAY = 0x62,
		NEW_MAP = 0x63,
		STORE_MAP = 0x64,

	};

	struct DlangObject
	{
		DlangType type;
		union
		{
			void* objPtr;
			int intValue;
			float floatValue;
			DWORD64 ptrValue;
		};

		std::vector<DlangObject>* arrayElements = nullptr; // Only used if type == DlangType::Array
		std::unordered_map<std::string, DlangObject>* mapElements = nullptr; // Only used if type == DlangType::Map
		
		DlangObject() : type(DlangType::None), ptrValue(0) {}
		DlangObject(int value, DlangType _type = DlangType::Integer) : type(_type), intValue(value) {}
		DlangObject(DWORD64 value) : type(DlangType::Pointer) { ptrValue = value; }
		DlangObject(float value) : type(DlangType::Float), floatValue(value) {}
		DlangObject(std::vector<DlangObject> elements) : type(DlangType::Array)
		{
			arrayElements = new std::vector<DlangObject>(std::move(elements));
		}
		DlangObject(std::unordered_map<std::string, DlangObject> elements) : type(DlangType::Map)
		{
			mapElements = new std::unordered_map<std::string, DlangObject>(std::move(elements));
		}

		inline std::string elementToString(DlangObject obj)
		{
			switch (obj.type)
			{
			case DlangType::Integer:
				return std::to_string(obj.intValue);
				break;
			case DlangType::Float:
				return std::to_string(obj.floatValue);
				break;
			case DlangType::Array:
				return "{ARRAY:" + std::to_string(obj.arrayElements->size()) + "}";
				break;
			case DlangType::Map:
				return "{MAP:" + std::to_string(obj.mapElements->size()) + "}";
			case DlangType::String:
				return "{STRING:" + std::to_string(obj.intValue) + "}";
				break;
			}
		}

		inline std::string arrayToString()
		{
			if (type != DlangType::Array || arrayElements == nullptr)
				throw std::runtime_error("Attempting to convert a non-array object to string.");

			std::stringstream ss;
			ss << "{ ";
			for (size_t i = 0; i < arrayElements->size(); i++)
				ss << elementToString((*arrayElements)[i]) << (i < arrayElements->size() - 1 ? ", " : " ");
			ss << "}[" << arrayElements->size() << "]";

			return ss.str();
		}

		inline std::string mapToString()
		{
			if (type != DlangType::Map || mapElements == nullptr)
				throw std::runtime_error("Attempting to convert a non-map object to string.");

			std::stringstream ss;
			ss << "{ ";
			size_t count = 0;
			for (const auto& pair : *mapElements)
			{
				ss << pair.first << ": " << elementToString(pair.second); // Assuming values are integers for simplicity
				if (count < mapElements->size() - 1)
					ss << ", ";
				count++;
			}
			ss << "}[" << mapElements->size() << "]";
			return ss.str();
		}
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