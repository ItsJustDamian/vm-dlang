#pragma once
#include "fnv.hpp"

namespace dlang
{
	namespace consts
	{
		static constexpr uint32_t KEYWORD_VAR = fnv1a_hash("var");
		static constexpr uint32_t KEYWORD_WHILE = fnv1a_hash("while");
		static constexpr uint32_t KEYWORD_IF = fnv1a_hash("if");
	}
}