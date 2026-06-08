#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include "../vm.hpp"

namespace dlang::functions::os
{
	inline void initFunctions(vm::DLangVirtualMachine* vm)
	{
		vm->registerNativeFunction("file_exists", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.file_exists, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			bool exists = std::filesystem::exists(pathString);
			vm->push(DlangObject(exists));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("read_file", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.read_file, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			std::ifstream file(pathString);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file: " + pathString);
			
			std::stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			
			int stringIndex = vm->addToStringPool(buffer.str());
			vm->push(DlangObject(stringIndex, DlangType::String));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("write_file", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String, DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.write_file, expected (string path, string content)");
			
			auto contentString = vm->getStringFromPool(vm->pop().intValue);
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			
			std::ofstream file(pathString);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file for writing: " + pathString);
			
			file << contentString;
			file.close();
			
			return true;
			}, "os", 2);

		vm->registerNativeFunction("delete_file", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.delete_file, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			bool removed = std::filesystem::remove(pathString);
			vm->push(DlangObject(removed));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("append_file", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String, DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.append_file, expected (string path, string content)");
			
			auto contentString = vm->getStringFromPool(vm->pop().intValue);
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			
			std::ofstream file(pathString, std::ios::app);
			if (!file.is_open())
				throw std::runtime_error("Failed to open file for appending: " + pathString);
			
			file << contentString;
			file.close();
			
			return true;
			}, "os", 2);

		vm->registerNativeFunction("create_directory", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.create_directory, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			bool created = std::filesystem::create_directory(pathString);
			vm->push(DlangObject(created));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("remove_directory", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.remove_directory, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			bool removed = std::filesystem::remove(pathString);
			vm->push(DlangObject(removed));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("list_directory", [](vm::DLangVirtualMachine* vm) -> bool {
			if (!vm->checkStack({ DlangType::String }))
				throw std::runtime_error("Invalid arguments for os.list_directory, expected (string path)");
			
			auto pathString = vm->getStringFromPool(vm->pop().intValue);
			std::vector<DlangObject> entries;
			for (const auto& entry : std::filesystem::directory_iterator(pathString))
			{
				int stringIndex = vm->addToStringPool(entry.path().string());
				entries.push_back(DlangObject(stringIndex, DlangType::String));
			}
			
			vm->push(DlangObject(entries));
			
			return true;
			}, "os", 1);

		vm->registerNativeFunction("get_current_directory", [](vm::DLangVirtualMachine* vm) -> bool {
			std::string currentPath = std::filesystem::current_path().string();
			int stringIndex = vm->addToStringPool(currentPath);
			vm->push(DlangObject(stringIndex, DlangType::String));
			
			return true;
			}, "os", 0);
	}
}