#pragma once
#include "fnv.hpp"

namespace dlang
{
	namespace consts
	{
		static constexpr uint32_t KEYWORD_VAR = fnv1a_hash("var");
		static constexpr uint32_t KEYWORD_WHILE = fnv1a_hash("while");
		static constexpr uint32_t KEYWORD_IF = fnv1a_hash("if");
		static constexpr uint32_t KEYWORD_FUNC = fnv1a_hash("func");
		static constexpr uint32_t KEYWORD_RETURN = fnv1a_hash("return");
		static constexpr uint32_t KEYWORD_ELSE = fnv1a_hash("else");
		static constexpr uint32_t KEYWORD_FOR = fnv1a_hash("for");
	}
}