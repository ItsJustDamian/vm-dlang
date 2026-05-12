#pragma once
#include <iostream>
#include <vector>
#include "consts.hpp"

namespace dlang
{
	namespace helpers
	{
		namespace memory
		{
			inline void writeInt32LE(int value, std::vector<uint8_t>& out)
			{
				out.push_back(value & 0xFF);
				out.push_back((value >> 8) & 0xFF);
				out.push_back((value >> 16) & 0xFF);
				out.push_back((value >> 24) & 0xFF);
			}

			inline void patchInt32LE(int32_t value, std::vector<uint8_t>& bytecode, size_t pos)
			{
				bytecode[pos] = static_cast<uint8_t>(value & 0xFF);
				bytecode[pos + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
				bytecode[pos + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
				bytecode[pos + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
			}

			inline bool readInt32LE(std::vector<uint8_t> data, int from, int* resultNumber)
			{
				if (data.size() < (from + 4))
					return false;

				*resultNumber = (data[from]) | (data[from + 1] << 8) | (data[from + 2] << 16) | (data[from + 3] << 24);
				return true;
			}
		}
	}
}