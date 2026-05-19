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
				auto it = std::find(m_stringPool.begin(), m_stringPool.end(), str);
				if (it != m_stringPool.end())
					return static_cast<int>(std::distance(m_stringPool.begin(), it));

				m_stringPool.push_back(str);
				return static_cast<int>(m_stringPool.size() - 1);
			}
		public:
			std::string getStringFromPool(int index)
			{
				if (index >= 0 && index < m_stringPool.size())
					return m_stringPool[index];
				else
					throw std::runtime_error("[VM]: String pool index out of bounds: " + std::to_string(index));
			}

			void storeVariable(const std::string& name, const DlangObject& value, const bool& localScope = false)
			{
				auto it = m_globalVariables.find(name);
				if (it != m_globalVariables.end() && !localScope)
				{
					it->second = value;
					return;
				}

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
					throw std::runtime_error("[VM]: Variable not found: " + name);
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
						throw std::runtime_error("[VM]: Instruction pointer out of bounds: " + std::to_string(i));

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
							throw std::runtime_error("[VM]: Failed to read integer from bytecode at position: " + std::to_string(i));
					} break;

					case Opcode::PUSH_FLOAT: {
						union {
							float f;
							int32_t i;
						} converter;

						if (helpers::memory::readInt32LE(bytes, i + 1, &converter.i))
						{
							push(DlangObject(converter.f));
							i += 4;
						}
						else
							throw std::runtime_error("[VM]: Failed to read float from bytecode at position: " + std::to_string(i));

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
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::String || right.type == DlangType::String) {
							auto convertToString = [&](DlangObject& obj) {
								if (obj.type == DlangType::String) return getStringFromPool(obj.intValue);
								if (obj.type == DlangType::Float)  return std::to_string(obj.floatValue); // Gebruik .floatValue!
								if (obj.type == DlangType::Array) return obj.arrayToString();
								if (obj.type == DlangType::Map) return obj.mapToString();
								return std::to_string(obj.intValue);
								};

							std::string sLeft = convertToString(left);
							std::string sRight = convertToString(right);

							push(DlangObject(addToStringPool(sLeft + sRight), DlangType::String));
						}
						else if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft + fRight));
						}
						else push(DlangObject(left.intValue + right.intValue));
					} break;

					case Opcode::SUB: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft - fRight));
						}
						else push(DlangObject(left.intValue - right.intValue));
					} break;

					case Opcode::MP: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft * fRight));
						}
						else push(DlangObject(left.intValue * right.intValue));
					} break;

					case Opcode::DIV: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							if(fRight == 0.0f)
								throw std::runtime_error("Division by zero error.");
							push(DlangObject(fLeft / fRight));
						}
						else {
							if(right.intValue == 0)
								throw std::runtime_error("Division by zero error.");
							push(DlangObject(left.intValue / right.intValue));
						}
					} break;

					case Opcode::STORE_VAR: {
						auto size = bytes[i + 1];
						auto varName = std::string(bytes.begin() + i + 2, bytes.begin() + i + 2 + size);
						i += size + 1;

						if (m_stack.size() < 1)
							throw std::runtime_error("[VM]: Not enough values on the stack to store variable: " + varName);

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
							throw std::runtime_error("[VM]: Failed to read jump address from bytecode at position: " + std::to_string(i));

						i = address - 1; // -1 because the loop will increment it
					} break;

					case Opcode::LESS_THAN: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft < fRight ? 1 : 0));
						}
						else push(DlangObject(left.intValue < right.intValue ? 1 : 0));
					} break;

					case Opcode::GREATER_THAN: {
						if(m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft > fRight ? 1 : 0));
						}
						else push(DlangObject(left.intValue > right.intValue ? 1 : 0));
					} break;

					case Opcode::LESS_OR_EQUAL_THAN: {
						if(m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft <= fRight ? 1 : 0));
						}
						else push(DlangObject(left.intValue <= right.intValue ? 1 : 0));
					} break;

					case Opcode::GREATER_OR_EQUAL_THAN: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft >= fRight ? 1 : 0));
						}
						else push(DlangObject(left.intValue >= right.intValue ? 1 : 0));
					} break;

					case Opcode::NOT_EQUALS_TO: {
						if(m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft != fRight ? 1 : 0));
						}
						else push(DlangObject(left.intValue != right.intValue ? 1 : 0));
					} break;

					case Opcode::COMPARE: {
						if (m_stack.size() < 2)
							throw std::runtime_error("[VM]: Not enough values on the stack to perform addition.");

						auto right = pop();
						auto left = pop();

						if (left.type == DlangType::Float || right.type == DlangType::Float) {
							float fLeft = (left.type == DlangType::Float) ? left.floatValue : static_cast<float>(left.intValue);
							float fRight = (right.type == DlangType::Float) ? right.floatValue : static_cast<float>(right.intValue);
							push(DlangObject(fLeft == fRight ? 1 : 0));
						}
						else push(left.intValue == right.intValue ? DlangObject(1) : DlangObject(0));
					} break;

					case Opcode::LOGIC_AND: {
						auto right = pop();
						auto left = pop();

						bool leftTrue = (left.type == DlangType::Float) ? (left.floatValue != 0.0f) : (left.intValue != 0);
						bool rightTrue = (right.type == DlangType::Float) ? (right.floatValue != 0.0f) : (right.intValue != 0);

						push(DlangObject(leftTrue && rightTrue ? 1 : 0));
					} break;

					case Opcode::LOGIC_OR: {
						auto right = pop();
						auto left = pop();

						bool leftBool = left.intValue != 0;
						bool rightBool = right.intValue != 0;

						push(DlangObject(leftBool || rightBool));
					} break;

					case Opcode::JUMP_IF_FALSE: {
						auto condition = pop();

						int target = -1;
						if(!helpers::memory::readInt32LE(bytes, i + 1, &target))
							throw std::runtime_error("[VM]: Failed to read jump address from bytecode at position: " + std::to_string(i));

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
							throw std::runtime_error("[VM]: Function not found: '" + funcName + "'");
						auto func = it->second;

						if(func.numArgs > m_stack.size())
							throw std::runtime_error("[VM]: Not enough arguments on the stack to call function: '" + funcName + "'");

						if (func.nativePtr != nullptr)
						{
							using fn = bool(*)(DLangVirtualMachine*);
							auto result = reinterpret_cast<fn>(func.nativePtr)(this);

							if(!result)
								throw std::runtime_error("[VM]: Native function '" + funcName + "' returned false, indicating an error during execution.");
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
							throw std::runtime_error("[VM]: Failed to read skipSize at " + std::to_string(cursor));
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
							throw std::runtime_error("[VM]: Return address stack is empty, cannot return from function.");

						// take back of return address and pop it
						i = m_returnAddressStack.back(); // Get the return address from the top of the return stack
						m_returnAddressStack.pop_back(); // Remove the return address from the stack
						m_scopeVariables.clear();
					} break;

					case Opcode::NEW_ARRAY: {
						auto countObj = pop();
						int elementCount = countObj.intValue;

						std::vector<DlangObject> elements;
						for (int i = 0; i < elementCount; i++)
							elements.push_back(pop());
						
						std::reverse(elements.begin(), elements.end());

						push(DlangObject(elements));
					} break;

					case Opcode::STORE_ARRAY: {
						auto indexObj = pop();
						auto value = pop();
						auto arrayObj = pop();
						int index = indexObj.intValue;

						if (arrayObj.type != DlangType::Array)
							throw std::runtime_error("[VM]: Attempting to index a non-array object.");

						if (arrayObj.arrayElements == nullptr)
							throw std::runtime_error("[VM]: Array object has null arrayElements pointer.");

						if (index < 0)
							throw std::runtime_error("[VM]: Array index out of bounds: " + std::to_string(index));
						
						if(index >= arrayObj.arrayElements->size())
							arrayObj.arrayElements->resize(index + 1);

						(*arrayObj.arrayElements)[index] = value;
					} break;

					case Opcode::LOAD_ARRAY: {
						auto indexObj = pop();
						auto arrayObj = pop();

						if (arrayObj.type == DlangType::Array)
						{
							if (indexObj.intValue < 0 || arrayObj.arrayElements == nullptr)
								throw std::runtime_error("[VM]: Array index out of bounds: " + std::to_string(indexObj.intValue));

							if(indexObj.intValue >= arrayObj.arrayElements->size())
								arrayObj.arrayElements->resize(indexObj.intValue + 1);

							push(arrayObj.arrayElements->at(indexObj.intValue));
						}
						else if (arrayObj.type == DlangType::Map)
						{
							std::string key = getStringFromPool(indexObj.intValue);

							if (arrayObj.mapElements == nullptr)
								throw std::runtime_error("[VM]: Map not initialized or is not map object: " + key);

							auto it = arrayObj.mapElements->find(key);
							if (it == arrayObj.mapElements->end())
								throw std::runtime_error("[VM]: Key not found in map: " + key);

							push(arrayObj.mapElements->at(key));
						}
						else if (arrayObj.type == DlangType::String)
						{
							std::string str = getStringFromPool(arrayObj.intValue);
							if (indexObj.intValue < 0 || indexObj.intValue >= str.size())
								throw std::runtime_error("[VM]: String index out of bounds: " + std::to_string(indexObj.intValue));
							char ch = str[indexObj.intValue];
							push(DlangObject(addToStringPool(std::string(1, ch)), DlangType::String));
						}
						else
							throw std::runtime_error("[VM]: Attempting to index a non-array/map/string object.");
						
					} break;

					case Opcode::NEW_MAP: {
						int pairCount = pop().intValue;
						std::unordered_map<std::string, DlangObject> elements;

						for (int i = 0; i < pairCount; i++) {
							auto value = pop();
							auto keyObj = pop();
							if (keyObj.type != DlangType::String)
								throw std::runtime_error("[VM]: Attempting to use a non-string key in a map.");
							std::string key = getStringFromPool(keyObj.intValue);
							elements[key] = value;
						}

						push(DlangObject(elements));
					} break;

					case Opcode::STORE_MAP: {
						auto valueObj = pop();
						auto indexObj = pop();
						auto mapObj = pop();

						if (mapObj.type == DlangType::Map)
						{
							if (mapObj.mapElements == nullptr)
								throw std::runtime_error("[VM]: Map object has null mapElements pointer.");

							std::string key;
							if (indexObj.type == DlangType::String) {
								key = getStringFromPool(indexObj.intValue);
							}
							else if (indexObj.type == DlangType::Integer) {
								key = std::to_string(indexObj.intValue);
							}
							else if (indexObj.type == DlangType::Float) {
								key = std::to_string(indexObj.floatValue);
							}
							else {
								throw std::runtime_error("[VM]: Attempting to use an invalid key type in a map.");
							}

							(*mapObj.mapElements)[key] = valueObj;
						}
						else if (mapObj.type == DlangType::Array)
						{
							if (mapObj.arrayElements == nullptr)
								throw std::runtime_error("[VM]: Array object has null arrayElements pointer.");

							int index = indexObj.intValue;
							if (index < 0)
								throw std::runtime_error("[VM]: Array index out of bounds during assignment: " + std::to_string(index));

							if(index >= static_cast<int>(mapObj.arrayElements->size()))
								mapObj.arrayElements->resize(index + 1);

							(*mapObj.arrayElements)[index] = valueObj;
						}
						else throw std::runtime_error("[VM]: Attempting to index a non-map/array object.");
					} break;

					default:
						throw std::runtime_error("[VM]: Unknown opcode: " + std::to_string(byte));
						break;
					}
				}
			}
		};
	}
}