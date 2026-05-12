#include <iostream>
#include <vector>
#include <chrono>
#include "dlang/vm.hpp"
#include "dlang/lexer.hpp"
#include "dlang/parser.hpp"

std::vector<std::uint8_t> testBytes = {
	dlang::Opcode::PUSH_INT, 0x00, 0x00, 0x00, 0x00,
	dlang::Opcode::STORE_VAR, 0x01, 'a',

	// Make it add 1 to a and loop back using jump
	dlang::Opcode::LOAD_VAR, 0x01, 'a',
	dlang::Opcode::PUSH_INT, 0x01, 0x00, 0x00, 0x00,
	dlang::Opcode::ADD,

	dlang::Opcode::STORE_VAR, 0x01, 'a',
	dlang::Opcode::JUMP, 0x08, 0x00, 0x00, 0x00,
	
	dlang::Opcode::EOC						  // EOC
};

void print(dlang::vm::DLangVirtualMachine* vm)
{
	while (vm->getStackSize() > 0)
	{
		auto obj = vm->pop();
		switch (obj.type)
		{
		case dlang::DlangType::Integer:
			printf("%i", obj.intValue);
			break;
		case dlang::DlangType::String:
			printf("%s", (*obj.strValue).c_str());
			break;
		case dlang::DlangType::Float:
			printf("%.001f", obj.floatValue);
			break;
		case dlang::DlangType::Boolean:
			printf("%s", (obj.boolValue ? "true" : "false"));
			break;
		}
	}
}

int main(int argc, char** argv)
{
	auto start = std::chrono::high_resolution_clock::now();

	dlang::vm::DLangVirtualMachine vm = dlang::vm::DLangVirtualMachine();
	vm.registerNativeFunction("print", (void*)print, 1);

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