#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class CMakeBuilder
{
private:
    bool ExecuteCommand(const std::string& command, const std::string& workingDir, std::string& outputLog)
    {
        HANDLE hReadPipe, hWritePipe;
        SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
            return false;

        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        PROCESS_INFORMATION pi = { 0 };
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;

        char cmdBuffer[1024];
        strcpy_s(cmdBuffer, command.c_str());

        if (!CreateProcessA(NULL, cmdBuffer, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE, NULL, workingDir.c_str(), &si, &pi))
        {
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            return false;
        }

        CloseHandle(hWritePipe);

        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            outputLog += buffer;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);

        return (exitCode == 0);
    }

public:
    bool GenerateBytecodeHeader(const std::string& ideLocation, const std::string& activeScriptPath, std::string& statusLog)
    {
        statusLog += "[BYTECODE] Requesting bytecode compilation from compiler core...\n";

        fs::path script(activeScriptPath.empty() ? "untitled.dlang" : activeScriptPath);
        if (script.is_relative()) {
            script = fs::current_path() / script;
        }

        std::string workingDir = script.parent_path().string();
        std::string cmd = "dlang.exe \"" + script.string() + "\" --save";

        std::string commandOutput = "";
        if (!ExecuteCommand(cmd, workingDir, commandOutput))
        {
            statusLog += commandOutput;
            statusLog += "ERROR: Failed to execute dlang.exe command for bytecode generation.\n";
            printf("Error executing dlang.exe for bytecode generation. Output:\n%s\n", commandOutput.c_str());
            return false;
        }

        fs::path generatedBytecode = script.parent_path() / "bytecode.hpp";
        fs::path destinationPath = fs::path(ideLocation) / "compiler_core" / "bytecode.hpp";

        if (!fs::exists(generatedBytecode))
        {
            statusLog += "ERROR: dlang.exe finished, but 'bytecode.hpp' was not generated in: " + workingDir + "\n";
            printf("[Bytecode] bytecode.hpp not found at: %s\n", workingDir.c_str());
            return false;
        }

        try
        {
            fs::create_directories(destinationPath.parent_path());
            fs::copy_file(generatedBytecode, destinationPath, fs::copy_options::overwrite_existing);
            fs::remove(generatedBytecode);

            statusLog += "[BYTECODE] Successfully bridged bytecode.hpp to IDE compiler_core.\n";
            return true;
        }
        catch (const std::exception& e)
        {
            statusLog += "ERROR: Failed to move bytecode.hpp to compiler_core: ";
            statusLog += e.what();
            statusLog += "\n";
            printf("[Bytecode] Exception while moving bytecode.hpp: %s\n", e.what());
            return false;
        }
    }

private:
    std::vector<std::string> requiredFiles = {
    "CMakeLists.txt",
    "entry.cpp",
    "bytecode.hpp",

    "dlang/dlang/vm.hpp",
    "dlang/dlang/structs.hpp",
    "dlang/dlang/parser.hpp",
    "dlang/dlang/lexer.hpp",
    "dlang/dlang/helpers.hpp",
    "dlang/dlang/fnv.hpp",
    "dlang/dlang/consts.hpp",

    "dlang/dlang/utils/graphics.hpp",
    "dlang/dlang/utils/utils.hpp",
    "dlang/dlang/utils/math.hpp",

    "dlang/dependencies/soloud/soloud.h",
    "dlang/dependencies/soloud/soloud.cpp",
    "dlang/dependencies/soloud/stb_vorbis.h"
    };
public:

    bool VerifyProjectStructure(const std::string& ideLocation, std::string& statusLog)
    {
        fs::path targetDir = fs::path(ideLocation) / "compiler_core";

        if (!fs::exists(targetDir))
        {
            statusLog += "ERROR: Target directory 'compiler_core' not found at IDE location: " + targetDir.string() + "\n";
            printf("[Verifier] Target directory not found: %s\n", targetDir.string().c_str());
            return false;
        }

        for (const auto& file : requiredFiles)
        {
            std::string normalizedFile = file;
            std::replace(normalizedFile.begin(), normalizedFile.end(), '\\', '/');

            if (!fs::exists(targetDir / normalizedFile))
            {
                statusLog += "ERROR: Missing required build component: " + normalizedFile + "\n";
                printf("[Verifier] Missing file: %s\n", normalizedFile.c_str());
                return false;
            }
        }

        statusLog += "[VERIFY] All compiler core components, utilities, and bytecode structures validated!\n";
        return true;
    }

public:
    bool CompileProject(const std::string& ideLocation, const std::string& activeScriptPath, std::string& statusLog)
    {
        fs::path targetDir = fs::path(ideLocation) / "compiler_core";
        std::string targetDirStr = targetDir.string();

        fs::create_directories(targetDir / "build");

        statusLog += "[BUILD] Generating CMake build configuration...\n";
        std::string configCommand = "cmake -B build -S .";
        std::string configOutput = "";

        if (!ExecuteCommand(configCommand, targetDirStr, configOutput))
        {
            statusLog += configOutput;
            statusLog += "ERROR: CMake configuration generation failed.\n";
            return false;
        }

        statusLog += "[BUILD] Compiling dlang architecture binary...\n";
        std::string buildCommand = "cmake --build build --config Release";
        std::string buildOutput = "";

        if (!ExecuteCommand(buildCommand, targetDirStr, buildOutput))
        {
            statusLog += buildOutput;
            statusLog += "ERROR: Compilation phase failed.\n";
            return false;
        }

        statusLog += buildOutput;
        statusLog += "[SUCCESS] Custom compiler compilation completed successfully!\n";

        try
        {
            fs::path scriptPath(GetProjectPrimaryFile());
            if (scriptPath.is_relative()) {
                scriptPath = fs::current_path() / scriptPath;
            }

            fs::path scriptFolder = scriptPath.parent_path();
            std::string folderName = scriptFolder.filename().string();

            if (folderName.empty()) {
                folderName = "output";
            }

            fs::path sourceExe = targetDir / "build" / "Release" / "dlang_compiler.exe";
            fs::path targetExe = scriptFolder / (folderName + ".exe");

            if (fs::exists(sourceExe))
            {
                statusLog += "[DEPLOY] Copying and renaming binary to: " + targetExe.string() + "\n";
                fs::copy_file(sourceExe, targetExe, fs::copy_options::overwrite_existing);
                return true;
            }
            else
            {
                statusLog += "ERROR: Compiled executable not found at expected location: " + sourceExe.string() + "\n";
                return false;
            }
        }
        catch (const std::exception& e)
        {
            statusLog += "ERROR: Failed to deploy executable to project folder: ";
            statusLog += e.what();
            statusLog += "\n";
            return false;
        }
    }
};