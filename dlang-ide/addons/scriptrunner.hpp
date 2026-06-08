#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

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