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
					throw std::runtime_error("Unexpected end of tokens at position: " + std::to_string(m_pos + offset));
			}

			lexer::Token consume()
			{
				if (m_pos < m_tokens.size())
					return m_tokens.at(m_pos++);
				else
					throw std::runtime_error("Unexpected end of tokens at position: " + std::to_string(m_pos));
			}

			bool isAtEnd() const
			{
				return m_pos >= m_tokens.size();
			}

		public:
			Parser(const std::vector<lexer::Token>& tokens) : m_tokens(tokens) { }

			void Parse()
			{
				while (!isAtEnd())
				{
					auto token = peek();
					auto tokenHash = fnv1a_hash(token.value.c_str());

					if (token.type == lexer::TokenType::KEYWORD && tokenHash == consts::KEYWORD_VAR)
						parseDeclaration();
					else if (token.type == lexer::TokenType::KEYWORD && tokenHash == consts::KEYWORD_WHILE)
						parseWhile();
					else if (token.type == lexer::TokenType::KEYWORD && tokenHash == consts::KEYWORD_IF)
						parseIf();
					else if (token.type == lexer::TokenType::KEYWORD && tokenHash == consts::KEYWORD_FUNC)
						parseFunctionDefinition();
					else if (token.type == lexer::TokenType::INCLUDE)
						parseInclude();
					else if (token.type == lexer::TokenType::IDENTIFIER && peek(1).value == "=")
						parseDeclaration(false);
					else
						parseExpression();
				}

				m_bytecode.push_back(Opcode::EOC); // End of code
			}

			std::vector<uint8_t> GetBytecode() const
			{
				return m_bytecode;
			}

		private:
			void emitString(const std::string& str)
			{
				m_bytecode.push_back(static_cast<uint8_t>(str.size()));
				for (char c : str)
					m_bytecode.push_back(static_cast<uint8_t>(c));
			}

			void parseDeclaration(bool hasVar = true)
			{
				if(hasVar) consume(); // Consume 'var' keyword
				auto id = consume(); // Consume identifier
				consume(); // Consume '=' operator

				parseExpression();

				m_bytecode.push_back(Opcode::STORE_VAR);
				emitString(id.value);
			}

			void parseFunctionCall(const std::string& funcName)
			{
				consume(); // Consume '(' token

				int argCount = 0;
				if (peek().value != ")") {
					do {
						parseExpression();
						argCount++;
						if (peek().value == ",") {
							consume();
						}
						else {
							break;
						}
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
					emitString(token.value.substr(1, token.value.size() - 2)); // Remove quotes
				}
				else if (token.type == lexer::TokenType::BOOLEAN)
				{
					m_bytecode.push_back(Opcode::PUSH_BOOL);
					m_bytecode.push_back(token.value == "true" ? 1 : 0);
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
				else if (token.type == lexer::TokenType::KEYWORD && token.value == "return")
					parseReturn();
				else if (token.type == lexer::TokenType::KEYWORD && token.value == "while")
					parseWhile();
				else if(token.type == lexer::TokenType::KEYWORD && token.value == "if")
					parseIf();
				else
					throw std::runtime_error("Unexpected token: " + token.value + " at line: " + std::to_string(token.line));
			}
			
			void parseMultiplication()
			{
				parsePrimary();

				while (!isAtEnd() && (peek().value[0] == '*' || peek().value[0] == '/'))
				{
					auto op = consume();
					parsePrimary();
					m_bytecode.push_back(op.value[0] == '*' ? Opcode::MP : Opcode::DIV);
				}
			}

			void parseAddition()
			{
				parseMultiplication();

				while (!isAtEnd() && (peek().value[0] == '+' || peek().value[0] == '-'))
				{
					auto op = consume();
					parseMultiplication();
					m_bytecode.push_back(op.value[0] == '+' ? Opcode::ADD : Opcode::SUB);
				}
			}

			void parseComparison()
			{
				parseAddition();

				while (!isAtEnd() && (peek().value[0] == '<' || 
									peek().value[0] == '>' ||
									peek().value == "==" ||
									peek().value == "!="))
				{
					auto op = consume();
					parseAddition();
					if (op.value.size() == 1 && op.value[0] == '<') m_bytecode.push_back(Opcode::LESS_THAN);
					if (op.value.size() == 1 && op.value[0] == '>') m_bytecode.push_back(Opcode::GREATER_THAN);
					if (op.value == "<=") m_bytecode.push_back(Opcode::LESS_OR_EQUAL_THAN);
					if (op.value == ">=") m_bytecode.push_back(Opcode::GREATER_OR_EQUAL_THAN);
					if (op.value == "==") m_bytecode.push_back(Opcode::COMPARE);
					if (op.value == "!=") m_bytecode.push_back(Opcode::NOT_EQUALS_TO);
					// later add other compare options
				}

			}

			
			void parseExpression()
			{
				parseLogicalAnd();
			}

			void parseLogicalAnd()
			{
				parseComparison();

				while (!isAtEnd() && peek().value == "&&")
				{
					consume();
					parseComparison();
					m_bytecode.push_back(Opcode::LOGIC_AND);
				}
			}

			void parseAssignment()
			{
				auto id = consume();

				if(peek().value[0] != '=')
					throw std::runtime_error("Expected '=' after identifier in assignment at line: " + std::to_string(id.line));

				consume(); // Consume '=' operator
				parseExpression();

				m_bytecode.push_back(Opcode::STORE_VAR);
				emitString(id.value);
			}

			void parseWhile()
			{
				if (peek().type == lexer::TokenType::KEYWORD && peek().value == "while") consume(); // Consume 'while' token
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
					auto t = peek();
					if (t.type == lexer::TokenType::KEYWORD && t.value == "var") parseDeclaration();
					else if (t.type == lexer::TokenType::IDENTIFIER && peek(1).value == "=") parseAssignment();
					else parseExpression();
				}

				consume(); // Consume '}' token

				m_bytecode.push_back(Opcode::JUMP);
				helpers::memory::writeInt32LE(static_cast<int32_t>(loopStart), m_bytecode);

				int32_t endAddress = static_cast<int32_t>(m_bytecode.size());
				helpers::memory::patchInt32LE(endAddress, m_bytecode, jumpIfFalsePos);
			}

			void parseIf()
			{
				if (peek().type == lexer::TokenType::KEYWORD && peek().value == "if") consume(); // Consume 'if' token
				consume(); // Consume '(' token
				parseExpression();
				consume(); // Consume ')' token
				consume(); // Consume '{' token

				m_bytecode.push_back(Opcode::JUMP_IF_FALSE);
				size_t jumpIfFalsePos = m_bytecode.size();
				for (int i = 0; i < 4; i++) m_bytecode.push_back(0);

				while(!isAtEnd() && peek().value[0 ] != '}')
				{
					auto t = peek();
					if (t.type == lexer::TokenType::KEYWORD && t.value == "var") parseDeclaration();
					else if (t.type == lexer::TokenType::IDENTIFIER && peek(1).value == "=") parseAssignment();
					else if (t.type == lexer::TokenType::IDENTIFIER && peek(1).value == "(")
					{
						std::string funcName = consume().value;
						parseFunctionCall(funcName);
					}
					else parseExpression();
				}

				consume(); // Consume '}' token

				int32_t endOfIf = static_cast<int32_t>(m_bytecode.size());
				helpers::memory::patchInt32LE(endOfIf, m_bytecode, jumpIfFalsePos);
			}

			void parseFunctionDefinition()
			{
				consume(); // Consume 'func' token
				auto funcNameToken = consume(); // Consume function name
				consume(); // Consume '(' token
				
				std::vector<std::string> paramNames;
				while (!isAtEnd() && peek().value[0] != ')')
				{
					auto paramToken = consume(); // Consume parameter name
					if (peek().value[0] == ',') consume(); // Consume ',' token if there are more parameters
					paramNames.push_back(paramToken.value);
				}

				consume(); // Consume ')' token
				consume(); // Consume '{' token

				m_bytecode.push_back(Opcode::DEF_FUNC);
				emitString(funcNameToken.value);
				m_bytecode.push_back(paramNames.size()); // Placeholder for number of arguments, update later when we add parameter parsing

				for (const auto& param : paramNames)
					emitString(param);

				size_t skipPatchPos = m_bytecode.size();
				helpers::memory::writeInt32LE(0, m_bytecode);

				while(!isAtEnd() && peek().value[0 ] != '}')
				{
					auto t = peek();
					if (t.type == lexer::TokenType::KEYWORD && t.value == "var") parseDeclaration();
					else if (t.type == lexer::TokenType::IDENTIFIER && peek(1).value == "=") parseAssignment();
					else if (t.type == lexer::TokenType::IDENTIFIER && peek(1).value == "(")
					{
						std::string funcName = consume().value;
						parseFunctionCall(funcName);
					}
					else parseExpression();
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
				auto filePathToken = consume(); // Consume file path string token
				consume(); // Consume ')' token

				lexer::Lexer includedLexer(filePathToken.value.substr(1, filePathToken.value.size() - 2));
				includedLexer.CompileInput();
				auto tokens = includedLexer.GetTokens();

				m_tokens.insert(m_tokens.begin() + m_pos, tokens.begin(), tokens.end());
			}
		};
	}
}