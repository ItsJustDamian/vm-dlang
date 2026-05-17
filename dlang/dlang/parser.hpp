#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "fnv.hpp"
#include "helpers.hpp"
#include "lexer.hpp"
#include "structs.hpp"

namespace dlang
{
	namespace parser
	{
		class Parser
		{
		private:
			std::vector<lexer::Token> m_tokens;
			std::vector<std::uint8_t> m_bytecode;
			size_t m_pos = 0;

			lexer::Token peek(size_t offset = 0) const
			{
				if (m_pos + offset < m_tokens.size())
					return m_tokens.at(m_pos + offset);
				else
					throw std::runtime_error("[PARSER]: Unexpected end of tokens.");
			}

			lexer::Token consume()
			{
				if (m_pos < m_tokens.size())
					return m_tokens.at(m_pos++);
				else
					throw std::runtime_error("[PARSER]: Unexpected end of tokens.");
			}

			bool isAtEnd() const { return m_pos >= m_tokens.size(); }

			std::string BinarySection()
			{
				std::stringstream ss;
				size_t start = m_bytecode.size() > 20 ? m_bytecode.size() - 20 : m_bytecode.size();
				for (size_t i = start; i < m_bytecode.size(); i++)
					ss << std::hex << static_cast<int>(m_bytecode[i]) << " ";
				return ss.str();
			}

			void emitString(const std::string& str)
			{
				m_bytecode.push_back(static_cast<uint8_t>(str.size()));
				for (char c : str)
					m_bytecode.push_back(static_cast<uint8_t>(c));
			}

		public:
			Parser(const std::vector<lexer::Token>& tokens) : m_tokens(tokens) {}

			void Parse()
			{
				while (!isAtEnd())
				{
					parseStatement();
				}
				m_bytecode.push_back(Opcode::EOC);
			}

			std::vector<uint8_t> GetBytecode() const { return m_bytecode; }

		private:
			void parseStatement()
			{
				if (isAtEnd()) return;

				auto token = peek();
				auto tokenHash = fnv1a_hash(token.value.c_str());

				if (token.type == lexer::TokenType::KEYWORD)
				{
					if (tokenHash == consts::KEYWORD_VAR) {
						parseDeclaration();
						return;
					}
					if (tokenHash == consts::KEYWORD_WHILE) {
						parseWhile();
						return;
					}
					if (tokenHash == consts::KEYWORD_IF) {
						parseIf();
						return;
					}
					if (tokenHash == consts::KEYWORD_FUNC) {
						parseFunctionDefinition();
						return;
					}
					if (token.value == "return") {
						consume(); // Consume 'return' token
						parseReturn();
						return;
					}
				}

				if (token.type == lexer::TokenType::INCLUDE) {
					parseInclude();
					return;
				}

				if (token.type == lexer::TokenType::IDENTIFIER)
				{
					if (peek(1).value == "=") {
						parseAssignment();
						return;
					}
					if (peek(1).type == lexer::TokenType::LBRACKET) {
						parseArrayMutationOrExpression();
						return;
					}
				}

				parseExpression();
			}

			void parseDeclaration()
			{
				consume(); // Consume 'var' token
				auto id = consume(); // Consume 'identifier' token

				if (consume().value != "=") { // Consume '=' token
					throw std::runtime_error("[PARSER]: Expected '=' in variable declaration.");
				}

				parseExpression();

				m_bytecode.push_back(Opcode::STORE_VAR);
				emitString(id.value);
			}

			void parseAssignment()
			{
				auto id = consume(); // Consume 'identifier' token
				consume(); // Consume '=' token

				parseExpression();

				m_bytecode.push_back(Opcode::STORE_VAR);
				emitString(id.value);
			}

			void parseArrayMutationOrExpression()
			{
				size_t savedPos = m_pos;

				auto id = consume(); // Consume 'identifier' token
				consume(); // Consume '[' token

				parseExpression();

				if (consume().type != lexer::TokenType::RBRACKET) { // Consume ']' token
					throw std::runtime_error("[PARSER]: Expected ']' in array mutation.");
				}

				if (!isAtEnd() && peek().value == "=")
				{
					consume(); // Consume '=' token
					parseExpression();

					m_bytecode.push_back(Opcode::LOAD_VAR);
					emitString(id.value);

					m_bytecode.push_back(0x64);
				}
				else
				{
					m_pos = savedPos;
					parseExpression();
				}
			}

			void parseFunctionCall(const std::string& funcName)
			{
				consume(); // Consume '(' token

				int argCount = 0;
				if (peek().value != ")") {
					do {
						parseExpression();
						argCount++;
						if (peek().value == ",") consume(); // Consume ',' token
						else break;
					} while (true);
				}

				consume(); // Consume ')' token
				m_bytecode.push_back(Opcode::CALL);
				emitString(funcName);
			}

			void parsePrimary()
			{
				auto token = consume();

				if (token.type == lexer::TokenType::FLOAT)
				{
					m_bytecode.push_back(Opcode::PUSH_FLOAT);
					float floatValue = std::stof(token.value);
					uint32_t intValue;
					std::memcpy(&intValue, &floatValue, sizeof(float));
					helpers::memory::writeInt32LE(intValue, m_bytecode);
				}
				else if (token.type == lexer::TokenType::NUMBER)
				{
					int32_t val = std::stoi(token.value);
					m_bytecode.push_back(Opcode::PUSH_INT);
					helpers::memory::writeInt32LE(val, m_bytecode);
				}
				else if (token.type == lexer::TokenType::STRING)
				{
					m_bytecode.push_back(Opcode::PUSH_STRING);
					emitString(token.value);
				}
				else if (token.type == lexer::TokenType::BOOLEAN)
				{
					m_bytecode.push_back(Opcode::PUSH_BOOL);
					m_bytecode.push_back(token.value == "true" ? 1 : 0);
				}
				else if (token.type == lexer::TokenType::SYMBOL && token.value[0] == '{')
				{
					int elementCount = 0;
					bool isNamedArray = false;

					if (!isAtEnd() && peek().type == lexer::TokenType::STRING && peek(1).value == ":")
						isNamedArray = true;

					while (peek().value[0] != '}')
					{
						if (isNamedArray)
						{
							auto keyToken = consume(); // Consume 'string' token
							if (keyToken.type != lexer::TokenType::STRING) {
								throw std::runtime_error("[PARSER]: Expected string key in named array.");
							}

							m_bytecode.push_back(Opcode::PUSH_STRING);
							emitString(keyToken.value);

							if (consume().value != ":") { // Consume ':' token
								throw std::runtime_error("[PARSER]: Expected ':' after key.");
							}

							parseExpression();
							elementCount++;
						}
						else {
							parseExpression();
							elementCount++;
						}

						if (peek().value[0] == ',')
						{
							consume(); // Consume ',' token
							if (!isAtEnd() && peek().value[0] == '}')
								break;
						}
						else if (peek().value[0] != '}') {
							throw std::runtime_error("[PARSER]: Expected ',' or '}' in array literal but got " + peek().value);
						}
					}

					consume(); // Consume '}' token

					m_bytecode.push_back(Opcode::PUSH_INT);
					helpers::memory::writeInt32LE(elementCount, m_bytecode);
					m_bytecode.push_back(isNamedArray ? Opcode::NEW_MAP : Opcode::NEW_ARRAY);
				}
				else if (token.type == lexer::TokenType::IDENTIFIER && peek().value[0] == '(')
				{
					parseFunctionCall(token.value);
				}
				else if (token.type == lexer::TokenType::IDENTIFIER)
				{
					m_bytecode.push_back(Opcode::LOAD_VAR);
					emitString(token.value);
				}
				else
					throw std::runtime_error("[PARSER]: Unexpected token: " + token.value + " Bytecode: " + BinarySection());
			}

			void parsePostfixExpression()
			{
				parsePrimary();

				while (!isAtEnd() && peek().type == lexer::TokenType::LBRACKET)
				{
					consume(); // Consume '[' token
					parseExpression();

					if (consume().type != lexer::TokenType::RBRACKET) { // Consume ']' token
						throw std::runtime_error("[PARSER]: Expected ']' after array index.");
					}

					m_bytecode.push_back(Opcode::LOAD_ARRAY);
				}
			}

			void parseMultiplication()
			{
				parsePostfixExpression();
				while (!isAtEnd() && (peek().value[0] == '*' || peek().value[0] == '/'))
				{
					auto op = consume(); // Consume 'operator' token
					parsePostfixExpression();
					m_bytecode.push_back(op.value[0] == '*' ? Opcode::MP : Opcode::DIV);
				}
			}

			void parseAddition()
			{
				parseMultiplication();
				while (!isAtEnd() && (peek().value[0] == '+' || peek().value[0] == '-'))
				{
					auto op = consume(); // Consume 'operator' token
					parseMultiplication();
					m_bytecode.push_back(op.value[0] == '+' ? Opcode::ADD : Opcode::SUB);
				}
			}

			void parseComparison()
			{
				parseAddition();
				while (!isAtEnd() && (peek().value[0] == '<' || peek().value[0] == '>' || peek().value == "==" || peek().value == "!="))
				{
					auto op = consume(); // Consume 'operator' token
					parseAddition();
					if (op.value == "<")  m_bytecode.push_back(Opcode::LESS_THAN);
					if (op.value == ">")  m_bytecode.push_back(Opcode::GREATER_THAN);
					if (op.value == "<=") m_bytecode.push_back(Opcode::LESS_OR_EQUAL_THAN);
					if (op.value == ">=") m_bytecode.push_back(Opcode::GREATER_OR_EQUAL_THAN);
					if (op.value == "==") m_bytecode.push_back(Opcode::COMPARE);
					if (op.value == "!=") m_bytecode.push_back(Opcode::NOT_EQUALS_TO);
				}
			}

			void parseLogicalAnd()
			{
				parseComparison();
				while (!isAtEnd() && peek().value == "&&")
				{
					consume(); // Consume '&&' token
					parseComparison();
					m_bytecode.push_back(Opcode::LOGIC_AND);
				}
			}

			void parseExpression()
			{
				parseLogicalAnd();
			}

			void parseWhile()
			{
				consume(); // Consume 'while' token
				consume(); // Consume '(' token

				size_t loopStart = m_bytecode.size();
				parseExpression();

				consume(); // Consume ')' token
				consume(); // Consume '{' token

				m_bytecode.push_back(Opcode::JUMP_IF_FALSE);
				size_t jumpIfFalsePos = m_bytecode.size();
				for (int i = 0; i < 4; i++) m_bytecode.push_back(0);

				while (!isAtEnd() && peek().value[0] != '}')
				{
					parseStatement();
				}

				consume(); // Consume '}' token
				m_bytecode.push_back(Opcode::JUMP);
				helpers::memory::writeInt32LE(static_cast<int32_t>(loopStart), m_bytecode);

				int32_t endAddress = static_cast<int32_t>(m_bytecode.size());
				helpers::memory::patchInt32LE(endAddress, m_bytecode, jumpIfFalsePos);
			}

			void parseIf()
			{
				consume(); // Consume 'if' token
				consume(); // Consume '(' token
				parseExpression();
				consume(); // Consume ')' token
				consume(); // Consume '{' token

				m_bytecode.push_back(Opcode::JUMP_IF_FALSE);
				size_t jumpIfFalsePos = m_bytecode.size();
				for (int i = 0; i < 4; i++) m_bytecode.push_back(0);

				while (!isAtEnd() && peek().value[0] != '}')
				{
					parseStatement();
				}

				consume(); // Consume '}' token
				int32_t endOfIf = static_cast<int32_t>(m_bytecode.size());
				helpers::memory::patchInt32LE(endOfIf, m_bytecode, jumpIfFalsePos);
			}

			void parseFunctionDefinition()
			{
				consume(); // Consume 'func' token
				auto funcNameToken = consume(); // Consume 'identifier' token
				consume(); // Consume '(' token

				std::vector<std::string> paramNames;
				while (!isAtEnd() && peek().value[0] != ')')
				{
					paramNames.push_back(consume().value); // Consume 'identifier' token
					if (peek().value[0] == ',') consume(); // Consume ',' token
				}

				consume(); // Consume ')' token
				consume(); // Consume '{' token

				m_bytecode.push_back(Opcode::DEF_FUNC);
				emitString(funcNameToken.value);
				m_bytecode.push_back(paramNames.size());

				for (const auto& param : paramNames)
					emitString(param);

				size_t skipPatchPos = m_bytecode.size();
				helpers::memory::writeInt32LE(0, m_bytecode);

				while (!isAtEnd() && peek().value[0] != '}')
				{
					parseStatement();
				}

				consume(); // Consume '}' token
				m_bytecode.push_back(Opcode::END_FUNC);

				uint32_t bodySize = static_cast<uint32_t>(m_bytecode.size() - (skipPatchPos + 4));
				helpers::memory::patchInt32LE(bodySize, m_bytecode, skipPatchPos);
			}

			void parseReturn()
			{
				if (peek().value != "}" && peek().value != ";") {
					parseExpression();
				}
				else {
					m_bytecode.push_back(Opcode::PUSH_INT);
					helpers::memory::writeInt32LE(0, m_bytecode);
				}
				m_bytecode.push_back(Opcode::RETURN);
			}

			void parseInclude()
			{
				consume(); // Consume 'include' token
				consume(); // Consume '(' token
				auto filePathToken = consume(); // Consume 'string' token
				consume(); // Consume ')' token

				lexer::Lexer includedLexer(filePathToken.value);
				includedLexer.CompileInput();
				auto tokens = includedLexer.GetTokens();

				m_tokens.insert(m_tokens.begin() + m_pos, tokens.begin(), tokens.end());
			}
		};
	}
}