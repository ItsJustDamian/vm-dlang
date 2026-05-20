#pragma once
#include <iostream>

static const char* entryTemplate = R"(
#include <Windows.h>
#include <iostream>

#include "dlang/dlang/vm.hpp"
#include "dlang/dlang/utils/utils.hpp"
#include "dlang/dlang/utils/graphics.hpp"
#include "dlang/dlang/utils/math.hpp"
#include "bytecode.hpp"

int main() {

    dlang::vm::DLangVirtualMachine vm = dlang::vm::DLangVirtualMachine();
    dlang::functions::utils::initFunctions(&vm);
    dlang::functions::graphics::initFunctions(&vm);
    dlang::functions::math::initFunctions(&vm);


    auto bytes = dlang::bytecode::getBytecode();
    
    if(dlang::bytecode::xorByte != 0xffffffff)
    {
        for(int i = 0; i < bytes.size(); i++) {
            bytes[i] ^= dlang::bytecode::xorByte;
        }
    }

    vm.eval(bytes);

    return 0;
}
)";