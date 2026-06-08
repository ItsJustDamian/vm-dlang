#include <iostream>
#include <vector>
#include <chrono>
#include "dlang/vm.hpp"
#include "dlang/lexer.hpp"
#include "dlang/parser.hpp"
#include "dlang/utils/utils.hpp"
#include "dlang/utils/graphics.hpp"
#include "dlang/utils/math.hpp"
#include "dlang/utils/os.hpp"

#include "tools/sha1.hpp"

int main(int argc, char** argv)
{
	if (argc < 1)
	{
		printf("Usage: %s <source_file>\n", argv[0]);
		return -1;
	}

	try {
		auto fileName = argv[1];
		auto start = std::chrono::high_resolution_clock::now();

		dlang::lexer::Lexer lexer = dlang::lexer::Lexer(fileName);
		lexer.CompileInput();

		dlang::parser::Parser parser = dlang::parser::Parser(lexer.GetTokens());
		parser.Parse();

		if (argc >= 3 && !strcmp(argv[2], "--save"))
		{
			int xorByte = -1;
			if (argc == 4)
				xorByte = std::stoi(argv[3]);

			printf("Saving bytecode to bytecode.hpp with XOR byte: %d\n", xorByte);

			auto byteCode = parser.GetBytecode();
			std::stringstream ss;
			ss << "#pragma once\n#include <vector>\n#include <cstdint>\n\nnamespace dlang {\n    namespace bytecode {\n		inline int xorByte = 0x" << std::hex << xorByte << ";\n        inline std::vector<uint8_t> getBytecode() {\n            return {";
			ss << std::hex;
			for (int i = 0; i < byteCode.size(); i++)
			{
				ss << "0x" << static_cast<int>((xorByte == -1 ? byteCode[i] : byteCode[i] ^ xorByte));
				if(i != byteCode.size() - 1)
					ss << ", ";

				printf("0x%02X ", (xorByte == -1 ? byteCode[i] : byteCode[i] ^ xorByte));
			}
			ss << "};\n        }\n    }\n}\n";

			std::ofstream f("bytecode.hpp");
			if (f.is_open())
			{
				f << ss.str();
				f.close();
			}

			std::string safeString;
			safeString.resize(byteCode.size());
			std::memcpy(&safeString[0], byteCode.data(), byteCode.size());

			SHA1 sha1;
			auto res = sha1.hash(safeString);

			printf("Bytecode saved to bytecode.hpp with SHA1 hash: %s\n", res.c_str());

			return 0;
		}

		dlang::vm::DLangVirtualMachine vm = dlang::vm::DLangVirtualMachine();
		dlang::functions::utils::initFunctions(&vm);
		dlang::functions::graphics::initFunctions(&vm);
		dlang::functions::math::initFunctions(&vm);
		dlang::functions::os::initFunctions(&vm);

		vm.eval(parser.GetBytecode());

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		printf("Execution time: %.6f seconds\n", elapsed.count());
	}
	catch (std::exception ex)
	{
		std::cerr << "Error during execution: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}