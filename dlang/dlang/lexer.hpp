#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include "fnv.hpp"
#include "helpers.hpp"

namespace dlang
{
	namespace lexer
	{
		enum TokenType : int
		{
			NONE = 0,
			KEYWORD = 1,
			IDENTIFIER = 2,
			OPERATOR = 3,
			NUMBER = 4,
			STRING = 5,
			FLOAT = 6,
			BOOLEAN = 7,
			SYMBOL = 8
		};

		struct Token
		{
			TokenType type = TokenType::NONE;
			std::string value = "";
			int line = 0;
		};

		class Lexer
		{
		private:
			std::string m_input, m_fileName;
			std::vector<Token> m_tokens;

		public:
			Lexer(const std::string& fileName) : m_fileName(fileName)
			{
				std::ifstream file(m_fileName);

				if(file.is_open())
				{
					std::stringstream buffer;
					buffer << file.rdbuf();
					m_input = buffer.str();
				}
				else
					throw std::runtime_error("Failed to open file: " + m_fileName);
			}

			void CompileInput()
			{
				std::string m_buffer;
				int lineNumber = 1;

				auto processBuffer = [&]() {
					if (!m_buffer.empty())
					{
						Token t;
						
						auto token = fnv1a_hash(m_buffer.c_str());

						if (m_buffer[0] == '=' || m_buffer[0] == '+' || m_buffer[0] == '-' || m_buffer[0] == '*' || m_buffer[0] == '/')
							t.type = TokenType::OPERATOR;
						else if (m_buffer[0] == '\'' || m_buffer[0] == '"')
							t.type = TokenType::STRING;
						else if (token == consts::KEYWORD_VAR ||
								token == consts::KEYWORD_WHILE || 
								token == consts::KEYWORD_IF ||
								token == consts::KEYWORD_FUNC ||
								token == consts::KEYWORD_RETURN)
							t.type = TokenType::KEYWORD;
						else if (isdigit(m_buffer[0]))
							t.type = TokenType::NUMBER;
						else if(m_buffer == "true" || m_buffer == "false")
							t.type = TokenType::BOOLEAN;
						else
							t.type = TokenType::IDENTIFIER;

						t.line = lineNumber;
						t.value = m_buffer;
						m_tokens.push_back(t);
						m_buffer.clear();
					}
				};

				for (int i = 0; i < m_input.size(); i++)
				{
					char c = m_input[i];
					if (c == ' ' || c == ';' || c == '\t')
						processBuffer();
					else if (c == '\n')
					{
						processBuffer();
						lineNumber++;
					}
					else if (c == '(' || c == ')' || c == '{' || c == '}' || c == ',')
					{
						processBuffer();
						Token t;
						t.type = TokenType::SYMBOL;
						t.value = std::string(1, c);
						t.line = lineNumber;
						m_tokens.push_back(t);
					}
					else if (c == '\'' || c == '"')
					{
						char quoteType = c;
						m_buffer += c;
						i++;
						while (i < m_input.size() && m_input[i] != quoteType)
						{
							if (m_input[i] == '\\' && i + 1 < m_input.size())
							{
								i++;
								char escapeChar = m_input[i];
								switch (escapeChar) {
								case 'n':  m_buffer += '\n'; break;
								case 't':  m_buffer += '\t'; break;
								case 'r':  m_buffer += '\r'; break;
								case '\\': m_buffer += '\\'; break;
								case '"':  m_buffer += '"';  break;
								case '\'': m_buffer += '\''; break;
								default:   m_buffer += escapeChar; break;
								}
							} else 
								m_buffer += m_input[i];
							i++;
						}
						if (i < m_input.size())
							m_buffer += quoteType; // Add closing quote
						else
							throw std::runtime_error("Unterminated string literal at line: " + std::to_string(lineNumber));
					}
					else
						m_buffer += c;
				}
				processBuffer();
			}

			const std::vector<Token>& GetTokens() const {
				return m_tokens;
			}
		};
	}
}