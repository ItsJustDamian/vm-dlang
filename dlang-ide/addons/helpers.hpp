#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "curl/curl.h"
#include "json.hpp"
#include <mutex>
#include "../templates/cmakelist.hpp"
#include "../templates/entry.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class ScriptRunner {
public:
    ~ScriptRunner() { if (worker.joinable()) worker.join(); }

    void Start(std::string activeFilePath) {
        if (running) return;

        if (worker.joinable()) {
            worker.join();
        }

        if (activeFilePath.empty()) {
            activeFilePath = "untitled.dlang";
        }

        running = true;
        output.clear();

        printf("[Runner] Start requested for: %s\n", activeFilePath.c_str());

        worker = std::thread(&ScriptRunner::Execute, this, activeFilePath);
    }

    std::string GetOutput() {
        std::lock_guard<std::mutex> lock(mtx);
        return output;
    }

    bool IsRunning() const { return running; }
	void SetRunning(bool value) { running = value; }

private:
    std::thread worker;
    std::mutex mtx;
    std::string output;
    bool running = false;

    std::string FilterAnsi(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        bool inEscape = false;

        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\x1b') {
                inEscape = true;
                continue;
            }
            if (inEscape) {
                if ((input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= 'a' && input[i] <= 'z')) {
                    inEscape = false;
                }
                continue;
            }
            result.push_back(input[i]);
        }
        return result;
    }

    void Execute(std::string filePath) {
        std::filesystem::path p(filePath);
        if (p.is_relative()) p = std::filesystem::current_path() / p;
        std::string absoluteFilePath = p.string();
        std::string workingDir = p.parent_path().string();
        std::string cmd = "dlang.exe \"" + absoluteFilePath + "\"";

        {
            std::lock_guard<std::mutex> lock(mtx);
            output = "[System]: Launching script...\n";
        }

        HANDLE hInPipeRead, hInPipeWrite;
        HANDLE hOutPipeRead, hOutPipeWrite;
        CreatePipe(&hInPipeRead, &hInPipeWrite, NULL, 0);
        CreatePipe(&hOutPipeRead, &hOutPipeWrite, NULL, 0);

        HPCON hPC = NULL;
        COORD size = { 80, 25 };
        HRESULT hr = CreatePseudoConsole(size, hInPipeRead, hOutPipeWrite, 0, &hPC);

        if (FAILED(hr)) {
            std::lock_guard<std::mutex> lock(mtx);
            output += "Error: ConPTY creation failed.\n";
            running = false;
            return;
        }

        STARTUPINFOEXA si = { 0 };
        si.StartupInfo.cb = sizeof(STARTUPINFOEXA);
        si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

        SIZE_T bytesRequired = 0;
        InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);
        si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, bytesRequired);
        InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &bytesRequired);
        UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(HPCON), NULL, NULL);

        PROCESS_INFORMATION pi = { 0 };
        std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
        cmdBuffer.push_back('\0');

        BOOL success = CreateProcessA(
            NULL, cmdBuffer.data(), NULL, NULL, TRUE,
            EXTENDED_STARTUPINFO_PRESENT,
            NULL, workingDir.c_str(), &si.StartupInfo, &pi
        );

        CloseHandle(hInPipeRead);
        CloseHandle(hOutPipeWrite);

        if (success) {
            char buf[1024];
            DWORD read;

            while (true) {
                DWORD bytesAvailable = 0;
                if (PeekNamedPipe(hOutPipeRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
                    if (ReadFile(hOutPipeRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
                        buf[read] = '\0';
                        std::string cleanChunk = FilterAnsi(std::string(buf, read));

                        std::lock_guard<std::mutex> lock(mtx);
                        output.append(cleanChunk);
                    }
                }

                if (WaitForSingleObject(pi.hProcess, 10) == WAIT_OBJECT_0) {
                    while (PeekNamedPipe(hOutPipeRead, NULL, 0, NULL, &bytesAvailable, NULL) && bytesAvailable > 0) {
                        if (ReadFile(hOutPipeRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
                            buf[read] = '\0';
                            std::string cleanChunk = FilterAnsi(std::string(buf, read));

                            std::lock_guard<std::mutex> lock(mtx);
                            output.append(cleanChunk);
                        }
                    }
                    break;
                }
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        ClosePseudoConsole(hPC);
        CloseHandle(hOutPipeRead);
        CloseHandle(hInPipeWrite);
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
        running = false;
    }
};

class CMakeDownloader
{
private:
    std::vector<std::string> filesToDownload = {
        "dlang\\vm.hpp",
        "dlang\\structs.hpp",
        "dlang\\parser.hpp",
        "dlang\\lexer.hpp",
        "dlang\\helpers.hpp",
        "dlang\\fnv.hpp",
        "dlang\\consts.hpp",
        "dlang\\utils\\graphics.hpp",
        "dlang\\utils\\utils.hpp",
        "dlang\\utils\\math.hpp",
        "dependencies\\soloud\\dr_flac.h",
        "dependencies\\soloud\\dr_impl.cpp",
        "dependencies\\soloud\\dr_mp3.h",
        "dependencies\\soloud\\dr_wav.h",
        "dependencies\\soloud\\soloud.cpp",
        "dependencies\\soloud\\soloud.h",
        "dependencies\\soloud\\soloud_audiosource.cpp",
        "dependencies\\soloud\\soloud_audiosource.h",
        "dependencies\\soloud\\soloud_bassboostfilter.h",
        "dependencies\\soloud\\soloud_biquadresonantfilter.h",
        "dependencies\\soloud\\soloud_bus.cpp",
        "dependencies\\soloud\\soloud_bus.h",
        "dependencies\\soloud\\soloud_c.h",
        "dependencies\\soloud\\soloud_core_3d.cpp",
        "dependencies\\soloud\\soloud_core_basicops.cpp",
        "dependencies\\soloud\\soloud_core_faderops.cpp",
        "dependencies\\soloud\\soloud_core_filterops.cpp",
        "dependencies\\soloud\\soloud_core_getters.cpp",
        "dependencies\\soloud\\soloud_core_setters.cpp",
        "dependencies\\soloud\\soloud_core_voicegroup.cpp",
        "dependencies\\soloud\\soloud_core_voiceops.cpp",
        "dependencies\\soloud\\soloud_dcremovalfilter.h",
        "dependencies\\soloud\\soloud_echofilter.h",
        "dependencies\\soloud\\soloud_error.h",
        "dependencies\\soloud\\soloud_fader.cpp",
        "dependencies\\soloud\\soloud_fader.h",
        "dependencies\\soloud\\soloud_fft.cpp",
        "dependencies\\soloud\\soloud_fft.h",
        "dependencies\\soloud\\soloud_fft_lut.cpp",
        "dependencies\\soloud\\soloud_fftfilter.h",
        "dependencies\\soloud\\soloud_file.cpp",
        "dependencies\\soloud\\soloud_file.h",
        "dependencies\\soloud\\soloud_file_hack_off.h",
        "dependencies\\soloud\\soloud_file_hack_on.h",
        "dependencies\\soloud\\soloud_filter.cpp",
        "dependencies\\soloud\\soloud_filter.h",
        "dependencies\\soloud\\soloud_flangerfilter.h",
        "dependencies\\soloud\\soloud_freeverbfilter.h",
        "dependencies\\soloud\\soloud_internal.h",
        "dependencies\\soloud\\soloud_lofifilter.h",
        "dependencies\\soloud\\soloud_misc.cpp",
        "dependencies\\soloud\\soloud_misc.h",
        "dependencies\\soloud\\soloud_monotone.h",
        "dependencies\\soloud\\soloud_noise.h",
        "dependencies\\soloud\\soloud_openmpt.h",
        "dependencies\\soloud\\soloud_queue.cpp",
        "dependencies\\soloud\\soloud_queue.h",
        "dependencies\\soloud\\soloud_robotizefilter.h",
        "dependencies\\soloud\\soloud_sfxr.h",
        "dependencies\\soloud\\soloud_speech.h",
        "dependencies\\soloud\\soloud_tedsid.h",
        "dependencies\\soloud\\soloud_thread.cpp",
        "dependencies\\soloud\\soloud_thread.h",
        "dependencies\\soloud\\soloud_vic.h",
        "dependencies\\soloud\\soloud_vizsn.h",
        "dependencies\\soloud\\soloud_wav.cpp",
        "dependencies\\soloud\\soloud_wav.h",
        "dependencies\\soloud\\soloud_waveshaperfilter.h",
        "dependencies\\soloud\\soloud_wavstream.cpp",
        "dependencies\\soloud\\soloud_wavstream.h",
        "dependencies\\soloud\\soloud_winmm.cpp",
        "dependencies\\soloud\\stb_vorbis.c",
        "dependencies\\soloud\\stb_vorbis.h"
    };

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static size_t WriteFileCallback(void* ptr, size_t size, size_t nmemb, FILE* stream)
    {
        return fwrite(ptr, size, nmemb, stream);
    }

    std::string FetchBuffer(const std::string& url)
    {
        CURL* curl = curl_easy_init();
        std::string readBuffer;

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "User-Agent: DLang-IDE-App");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }

    bool DownloadRawFile(const std::string& filename, const fs::path& targetPath)
    {
        fs::create_directories(targetPath.parent_path());

        CURL* curl = curl_easy_init();
        if (!curl) return false;

        FILE* fp = NULL;
        fopen_s(&fp, targetPath.string().c_str(), "wb");
        if (!fp)
        {
            curl_easy_cleanup(curl);
            return false;
        }

        std::string normalizedFilename = filename;
        std::replace(normalizedFilename.begin(), normalizedFilename.end(), '\\', '/');

        auto url = "https://raw.githubusercontent.com/ItsJustDamian/vm-dlang/refs/heads/master/dlang/" + normalizedFilename;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: DLang-IDE-App");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        fclose(fp);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return (res == CURLE_OK);
    }

    bool GenerateCMakeProject(const fs::path& targetDir)
    {
        std::ofstream cmakeFile(targetDir / "CMakeLists.txt");
        if (cmakeFile.is_open())
        {
            cmakeFile << cmakeTemplate;
            cmakeFile.close();

			std::ofstream entryFile(targetDir / "entry.cpp");
            if (entryFile.is_open())
            {
                entryFile << entryTemplate;
                entryFile.close();
                return true;
            }
        }

        return false;
    }

public:
    bool DownloadBuildBridge(const std::string& projectRoot, std::string& statusLog)
    {
        fs::path targetDir = fs::path(projectRoot) / "compiler_core/dlang";
        fs::create_directories(targetDir);

        for (const auto& file : filesToDownload)
        {
            if (!DownloadRawFile(file, targetDir / file))
            {
                statusLog += "ERROR: Failed to download required build bridge file: " + file + "\n";
                printf("[Downloader] Failed to download: %s\n", file.c_str());
                return false;
            }
        }

        if (!GenerateCMakeProject(fs::path(projectRoot) / "compiler_core"))
        {
            statusLog += "ERROR: Failed to generate CMakeLists.txt\n";
            printf("[Downloader] Failed to generate CMakeLists.txt\n");
            return false;
        }

        printf("[Downloader] Successfully downloaded build bridge files and generated CMakeLists.txt.\n");
        statusLog += "[DOWNLOAD] Successfully downloaded build bridge files and generated CMakeLists.txt.\n";
        return true;
    }
};

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
            fs::path scriptPath(activeScriptPath.empty() ? "untitled.dlang" : activeScriptPath);
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