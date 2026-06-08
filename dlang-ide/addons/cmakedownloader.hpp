#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "curl/curl.h"
#include "json.hpp"
#include "../templates/cmakelist.hpp"
#include "../templates/entry.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "libcurl.lib")

namespace fs = std::filesystem;
using json = nlohmann::json;

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