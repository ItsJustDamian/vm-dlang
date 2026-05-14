#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "helpers.hpp"
#include "structs.hpp"

namespace dlang
{
	namespace vm
	{
		class DLangVirtualMachine
		{
		private:
			std::vector<DlangObject> m_stack;
			std::unordered_map<std::string, DlangObject> m_globalVariables;
			std::unordered_map<std::string, DlangObject> m_scopeVariables;
			std::unordered_map<std::string, DlangFunction> m_functions;
			std::vector<std::string> m_stringPool;
			std::vector<int> m_returnAddressStack;
			int m_stackPointer = -1;

		public:
			DLangVirtualMachine() = default;

			void push(const DlangObject& obj)
			{
				m_stack.push_back(obj);
				m_stackPointer++;
			}

			DlangObject pop(int index = 0)
			{
				if (m_stackPointer - index >= 0)
				{
					auto obj = m_stack[m_stackPointer - index];
					m_stack.erase(m_stack.end() - 1 - index);
					m_stackPointer--;
					return obj;
				}

				return DlangObject();
			}

			int top()
			{
				return m_stack.size() - 1;
			}

			void debugStack()
			{
				for (const auto& item : m_stack)
				{
					switch (item.type)
					{
					case DlangType::Integer:
						std::cout << "Integer: " << item.intValue << std::endl;
						break;
					case DlangType::String:
						std::cout << "String: " << getStringFromPool(item.intValue) << std::endl;
						break;
					default:
						std::cout << "Unknown type on stack." << std::endl;
						break;
					}
				}
			}

			bool isType(DlangType type, int stackIndex = 0)
			{
				if (m_stack.size() > stackIndex)
					return m_stack[m_stackPointer - stackIndex].type == type;
				else
					return false;
			}

			bool checkStack(const std::vector<DlangType> expectedTypes)
			{
				if (m_stack.size() < expectedTypes.size())
					return false;
				for (int i = 0; i < expectedTypes.size(); i++)
				{
					if (m_stack[m_stackPointer - i].type != expectedTypes[i])
						return false;
				}
				return true;
			}

		private:
			int addToStringPool(const std::string& str)
			{
				m_stringPool.push_back(str);
				return static_cast<int>(m_stringPool.size() - 1);
			}
		public:
			std::string getStringFromPool(int index)
			{
				if (index >= 0 && index < m_stringPool.size())
					return m_stringPool[index];
				else
					throw std::runtime_error("String pool index out of bounds: " + std::to_string(index));
			}

			void storeVariable(const std::string& name, const DlangObject& value, const bool& localScope = false)
			{
				if(m_returnAddressStack.size() > 0 || localScope)
					m_scopeVariables[name] = value;
				else
					m_globalVariables[name] = value;
			}

			DlangObject loadVariable(const std::string& name)
			{
				if (m_scopeVariables.find(name) != m_scopeVariables.end())
					return m_scopeVariables[name];
				
				if (m_globalVariables.find(name) != m_globalVariables.end())
					return m_globalVariables[name];
				else
					throw std::runtime_error("Variable not found: " + name);
			}

			/* Namespace * = global, any other is whatever you assign */
			void registerNativeFunction(const std::string& name, NativeCallback function, std::string nameSpace = "*", int numArgs = 0)
			{
				DlangFunction func;
				func.isNative = true;
				func.nativePtr = function;
				func.numArgs = numArgs;
				func.name = name;
				m_functions[(nameSpace == "*" ? name : nameSpace + "." + name)] = func;
			}

			int getStackSize() const
			{
				return m_stack.size();
			}

			void eval(const std::vector<std::uint8_t>& bytes)
			{
				for (int i = 0; i < bytes.size(); i++)
				{
					if(i > bytes.size())
						throw std::runtime_error("Instruction pointer out of bounds: " + std::to_string(i));

					const auto& byte = bytes[i];

					switch (byte)
					{
					case Opcode::EOC:
						return;
						break;

					case Opcode::PUSH_INT: {
						int resultNumber = 0;
						if (helpers::memory::readInt32LE(bytes, i + 1, &resultNumber))
						{
							push(DlangObject(resultNumber));
							i += 4;
						}
						else
							throw std::runtime_error("Failed to read integer from bytecode at position: " + std::to_string(i));
					} break;

					case Opcode::PUSH_STRING: {
						uint8_t size = bytes[i + 1];
						std::string val = std::string(bytes.begin() + i + 2, bytes.begin() + i + 2 + size);
						i += size + 1;
						push(DlangObject(addToStringPool(val), DlangType::String));
					} break;

					case Opcode::PUSH_BOOL: {
						auto val = bytes[i+1] != 0;
						i++;
						push(DlangObject(val ? 1 : 0));
					} break;

					case Opcode::ADD: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::String || right.type == DlangType::String) {
							std::string sLeft = (left.type == DlangType::String) ? getStringFromPool(left.intValue) : std::to_string(left.intValue);
							std::string sRight = (right.type == DlangType::String) ? getStringFromPool(right.intValue) : std::to_string(right.intValue);

							push(DlangObject(addToStringPool(sLeft + sRight), DlangType::String));
						}
						else push(DlangObject(left.intValue + right.intValue));
					} break;

					case Opcode::SUB: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto obj1 = pop();
						auto obj2 = pop();

						push(DlangObject(obj1.intValue - obj2.intValue));
					} break;

					case Opcode::MP: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto obj1 = pop();
						auto obj2 = pop();

						push(DlangObject(obj1.intValue * obj2.intValue));
					} break;

					case Opcode::DIV: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto obj1 = pop();
						auto obj2 = pop();

						push(DlangObject(obj1.intValue / obj2.intValue));
					} break;

					case Opcode::STORE_VAR: {
						auto size = bytes[i + 1];
						auto varName = std::string(bytes.begin() + i + 2, bytes.begin() + i + 2 + size);
						i += size + 1;

						if (m_stack.size() < 1)
							throw std::runtime_error("Not enough values on the stack to store variable: " + varName);

						auto value = pop();
						storeVariable(varName, value);
					} break;

					case Opcode::LOAD_VAR: {
						auto size = bytes[i + 1];
						auto varName = std::string(bytes.begin() + i + 2, bytes.begin() + i + 2 + size);
						i += size + 1;

						push(loadVariable(varName));
					} break;

					case Opcode::JUMP: {
						int address = 0;
						if (!helpers::memory::readInt32LE(bytes, i + 1, &address))
							throw std::runtime_error("Failed to read jump address from bytecode at position: " + std::to_string(i));

						i = address - 1; // -1 because the loop will increment it
					} break;

					case Opcode::LESS_THAN: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(DlangObject(left.intValue < right.intValue ? 1 : 0));
					} break;

					case Opcode::GREATER_THAN: {
						if(m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(DlangObject(left.intValue > right.intValue ? 1 : 0));
					} break;

					case Opcode::LESS_OR_EQUAL_THAN: {
						if(m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(DlangObject(left.intValue <= right.intValue ? 1 : 0));
					} break;

					case Opcode::GREATER_OR_EQUAL_THAN: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(DlangObject(left.intValue >= right.intValue ? 1 : 0));
					} break;

					case Opcode::NOT_EQUALS_TO: {
						if(m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(DlangObject(left.intValue != right.intValue ? 1 : 0));
					} break;

					case Opcode::COMPARE: {
						if (m_stack.size() < 2)
							throw std::runtime_error("Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						push(left.intValue == right.intValue ? DlangObject(1) : DlangObject(0));
					} break;

					case Opcode::JUMP_IF_FALSE: {
						auto condition = pop();

						int target = -1;
						if(!helpers::memory::readInt32LE(bytes, i + 1, &target))
							throw std::runtime_error("Failed to read jump address from bytecode at position: " + std::to_string(i));

						if (condition.intValue == 0)
							i = target - 1;
						else
							i += 4;
					} break;

					case Opcode::CALL: {
						auto size = bytes[i + 1];
						auto funcName = std::string(bytes.begin() + i + 2, bytes.begin() + i + 2 + size);
						i += size + 1;

						auto it = m_functions.find(funcName);
						if(it == m_functions.end())
							throw std::runtime_error("Function not found: '" + funcName + "'");
						auto func = it->second;

						if(func.numArgs > m_stack.size())
							throw std::runtime_error("Not enough arguments on the stack to call function: '" + funcName + "'");

						if (func.nativePtr != nullptr)
						{
							using fn = bool(*)(DLangVirtualMachine*);
							auto result = reinterpret_cast<fn>(func.nativePtr)(this);

							if(!result)
								throw std::runtime_error("Native function '" + funcName + "' returned false, indicating an error during execution.");
						}
						else {
							for (int i = 0; i < func.numArgs; i++)
							{
								auto val = pop();
								auto name = func.argNames[func.numArgs - 1 - i];

								storeVariable(name, val, true);
							}

							m_returnAddressStack.push_back(i++); // Push the return address (next instruction) onto the return stack
							i = func.startInstruction-1; // Jump to the function's starting instruction
						}
					} break;

					case Opcode::DEF_FUNC: {
						size_t cursor = i + 1;

						uint8_t nameSize = bytes[cursor++];
						std::string funcName(bytes.begin() + cursor, bytes.begin() + cursor + nameSize);
						cursor += nameSize;

						uint8_t argCount = bytes[cursor++];

						std::vector<std::string> argNames;
						for (int j = 0; j < argCount; j++) {
							uint8_t argNameSize = bytes[cursor++];
							std::string argName(bytes.begin() + cursor, bytes.begin() + cursor + argNameSize);
							argNames.push_back(argName);
							cursor += argNameSize;
						}

						int32_t skipSize = 0;
						if (!helpers::memory::readInt32LE(bytes, cursor, &skipSize))
							throw std::runtime_error("Failed to read skipSize at " + std::to_string(cursor));
						cursor += 4;

						DlangFunction func;
						func.isNative = false;
						func.name = funcName;
						func.numArgs = argCount;
						func.argNames = argNames;
						func.bodySize = skipSize;
						func.startInstruction = cursor;
						m_functions[funcName] = func;

						i = cursor + skipSize - 1;

					} break;

					case Opcode::RETURN:
					case Opcode::END_FUNC: {
						if (m_returnAddressStack.empty())
							throw std::runtime_error("Return address stack is empty, cannot return from function.");

						// take back of return address and pop it
						i = m_returnAddressStack.back(); // Get the return address from the top of the return stack
						m_returnAddressStack.pop_back(); // Remove the return address from the stack
						m_scopeVariables.clear();
					} break;

					default:
						throw std::runtime_error("Unknown opcode: " + std::to_string(byte));
						break;
					}
				}
			}
		};
	}
}