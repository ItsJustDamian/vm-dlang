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
			inline void writeInt32LE(uint32_t value, std::vector<uint8_t>& out)
			{
				out.push_back(static_cast<uint8_t>(value & 0xFF));
				out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
				out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
				out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
			}

			inline void patchInt32LE(int32_t value, std::vector<uint8_t>& bytecode, size_t pos)
			{
				bytecode[pos] = static_cast<uint8_t>(value & 0xFF);
				bytecode[pos + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
				bytecode[pos + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
				bytecode[pos + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
			}

			inline bool readInt32LE(const std::vector<uint8_t>& data, int from, int* resultNumber)
			{
				if (data.size() < (size_t)(from + 4))
					return false;

				uint32_t b1 = static_cast<uint32_t>(data[from]);
				uint32_t b2 = static_cast<uint32_t>(data[from + 1]);
				uint32_t b3 = static_cast<uint32_t>(data[from + 2]);
				uint32_t b4 = static_cast<uint32_t>(data[from + 3]);

				uint32_t finalValue = b1 | (b2 << 8) | (b3 << 16) | (b4 << 24);

				*resultNumber = static_cast<int>(finalValue);
				return true;
			}
		}
	}
}