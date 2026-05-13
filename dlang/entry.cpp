#include <iostream>
#include <vector>
#include <chrono>
#include "dlang/vm.hpp"
#include "dlang/lexer.hpp"
#include "dlang/parser.hpp"
#include "dlang/utils/utils.hpp"

int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	dlang::vm::DLangVirtualMachine vm = dlang::vm::DLangVirtualMachine();
	dlang::functions::utils::initFunctions(&vm);

	dlang::lexer::Lexer lexer = dlang::lexer::Lexer("test.dlang");
	lexer.CompileInput();

	dlang::parser::Parser parser = dlang::parser::Parser(lexer.GetTokens());

	try {
		parser.Parse();
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