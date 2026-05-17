#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cctype>
#include "fnv.hpp"
#include "helpers.hpp"

namespace dlang
{
	namespace lexer
	{
		enum class TokenType : int
		{
			NONE = 0,
			KEYWORD,
			IDENTIFIER,
			OPERATOR,
			NUMBER,
			STRING,
			FLOAT,
			BOOLEAN,
			SYMBOL,
			INCLUDE,
			LBRACKET,
			RBRACKET,
			DOT
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
			std::string m_input;
			std::string m_fileName;
			std::vector<Token> m_tokens;
			size_t m_index = 0;
			int m_line = 1;

			char peek(size_t offset = 0) const
			{
				if (m_index + offset >= m_input.size()) return '\0';
				return m_input[m_index + offset];
			}

			char advance()
			{
				if (m_index >= m_input.size()) return '\0';
				char c = m_input[m_index++];
				if (c == '\n') m_line++;
				return c;
			}

			void skipWhitespace()
			{
				while (m_index < m_input.size())
				{
					char c = peek();
					if (c == ' ' || c == '\t' || c == '\r' || c == ';')
					{
						advance();
					}
					else if (c == '\n')
					{
						advance();
					}
					else
					{
						break;
					}
				}
			}

			void readNumber()
			{
				std::string buffer;
				bool isFloat = false;

				if (peek() == '-')
				{
					buffer += advance();
				}

				while (std::isdigit(peek()))
				{
					buffer += advance();
				}

				if (peek() == '.' && std::isdigit(peek(1)))
				{
					isFloat = true;
					buffer += advance(); // '.'
					while (std::isdigit(peek()))
					{
						buffer += advance();
					}
				}

				TokenType type = isFloat ? TokenType::FLOAT : TokenType::NUMBER;
				m_tokens.push_back({ type, buffer, m_line });
			}

			void readIdentifierOrKeyword()
			{
				std::string buffer;

				while (std::isalnum(peek()) || peek() == '_')
				{
					buffer += advance();
				}

				while (peek() == '.' && (std::isalpha(peek(1)) || peek(1) == '_'))
				{
					buffer += advance();

					while (std::isalnum(peek()) || peek() == '_')
					{
						buffer += advance();
					}
				}

				auto hashValue = fnv1a_hash(buffer.c_str());

				if (hashValue == consts::KEYWORD_VAR ||
					hashValue == consts::KEYWORD_WHILE ||
					hashValue == consts::KEYWORD_IF ||
					hashValue == consts::KEYWORD_FUNC ||
					hashValue == consts::KEYWORD_RETURN)
				{
					m_tokens.push_back({ TokenType::KEYWORD, buffer, m_line });
				}
				else if (buffer == "true" || buffer == "false")
				{
					m_tokens.push_back({ TokenType::BOOLEAN, buffer, m_line });
				}
				else if (buffer == "include")
				{
					m_tokens.push_back({ TokenType::INCLUDE, buffer, m_line });
				}
				else
				{
					m_tokens.push_back({ TokenType::IDENTIFIER, buffer, m_line });
				}
			}

			void readString(char quoteType)
			{
				if (peek() == quoteType)
					m_index++;

				std::string buffer;
				while (m_index < m_input.size() && m_input[m_index] != quoteType)
				{
					char current = m_input[m_index];

					if (current == '\\' && m_index + 1 < m_input.size())
					{
						m_index++; // Skip de backslash
						char escapeChar = m_input[m_index];
						switch (escapeChar)
						{
						case 'n':  buffer += '\n'; break;
						case 't':  buffer += '\t'; break;
						case 'r':  buffer += '\r'; break;
						case '\\': buffer += '\\'; break;
						case '"':  buffer += '"';  break;
						case '\'': buffer += '\''; break;
						default:   buffer += escapeChar; break;
						}
						m_index++;
					}
					else
					{
						buffer += current;
						m_index++;
					}
				}

				if (m_index >= m_input.size())
				{
					throw std::runtime_error("[LEXER]: Unterminated string literal at line: " + std::to_string(m_line));
				}

				m_index++;
				m_tokens.push_back({ TokenType::STRING, buffer, m_line });
			}

		public:
			Lexer(const std::string& fileName) : m_fileName(fileName)
			{
				std::ifstream file(m_fileName);
				if (file.is_open())
				{
					std::stringstream buffer;
					buffer << file.rdbuf();
					m_input = buffer.str();
				}
				else
				{
					throw std::runtime_error("[LEXER]: Failed to open file: " + m_fileName);
				}
			}

			void CompileInput()
			{
				m_tokens.clear();
				m_index = 0;
				m_line = 1;

				while (m_index < m_input.size())
				{
					skipWhitespace();
					if (m_index >= m_input.size()) break;

					char c = peek();

					if (std::isdigit(c) || (c == '-' && std::isdigit(peek(1))))
					{
						readNumber();
						continue;
					}

					if (std::isalpha(c) || c == '_')
					{
						readIdentifierOrKeyword();
						continue;
					}

					if (c == '"' || c == '\'')
					{
						readString(c);
						continue;
					}

					if (c == '=' || c == '!' || c == '+' || c == '-' || c == '*' || c == '/' || c == '>' || c == '<')
					{
						std::string op;
						op += advance();

						if (peek() == '=')
							op += advance();

						m_tokens.push_back({ TokenType::OPERATOR, op, m_line });
						continue;
					}

					if (c == '&')
					{
						advance();
						if (peek() == '&')
						{
							advance();
							m_tokens.push_back({ TokenType::OPERATOR, "&&", m_line });
						}
						else
						{
							throw std::runtime_error("[LEXER]: Unexpected character '&' at line: " + std::to_string(m_line) + ". Did you mean '&&'?");
						}
						continue;
					}

					if (c == '[' || c == ']')
					{
						TokenType type = (c == '[') ? TokenType::LBRACKET : TokenType::RBRACKET;
						m_tokens.push_back({ type, std::string(1, advance()), m_line });
						continue;
					}

					if (c == '(' || c == ')' || c == '{' || c == '}' || c == ',' || c == ':')
					{
						m_tokens.push_back({ TokenType::SYMBOL, std::string(1, advance()), m_line });
						continue;
					}

					throw std::runtime_error("[LEXER]: Unknown character '" + std::string(1, c) + "' at line: " + std::to_string(m_line));
				}
			}

			const std::vector<Token>& GetTokens() const
			{
				return m_tokens;
			}
		};
	}
}