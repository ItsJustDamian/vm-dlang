#include <filesystem>
#include <fstream>
#include <shobjidl.h>
#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "imgui-addons/TextEditor.h"
#include "addons/json.hpp"
#include "addons/helpers.hpp"

#pragma comment(lib, "d3d11.lib")

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace fs = std::filesystem;
inline std::string currentProjectPath = "";
inline std::string activeFilePath = "";
inline std::string consoleOutput = "";
inline std::string ideLocation = "";
inline bool g_isWindowMaximized = false;

class Settings
{
public:
    nlohmann::json settingData;

    void load()
    {
        std::ifstream i("settings.json");
        if (i.is_open())
        {
            i >> settingData;
        }
        else
        {
            settingData["window"]["width"] = 1000;
            settingData["window"]["height"] = 700;
            settingData["editor"]["font_size"] = 16.0f;
            settingData["editor"]["theme"] = "dark";
            settingData["last_project"] = "";
            settingData["last_file"] = "";
            save();
        }
    }

    void save()
    {
        std::ofstream o("settings.json");
        o << settingData.dump(4);
    }
} inline settings;

std::string ChooseFolderDialog()
{
    std::string resultPath = "";
    IFileDialog* pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
    {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
        {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
        if (SUCCEEDED(pfd->Show(NULL)))
        {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi)))
            {
                PWSTR pszFilePath = NULL;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                {
                    std::wstring ws(pszFilePath);
                    resultPath = std::string(ws.begin(), ws.end());
                    CoTaskMemFree(pszFilePath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return resultPath;
}

inline fs::path g_browserFocusedPath = "";
inline fs::path g_popupTargetPath = "";
inline char g_popupNameBuffer[128] = "";
inline bool g_openNewFileModal = false;
inline bool g_openNewFolderModal = false;
inline bool g_openRenameModal = false;

void RenderProjectTree(const fs::path& path, TextEditor& editor)
{
    try
    {
        for (const auto& entry : fs::directory_iterator(path))
        {
            auto entryPath = entry.path();
            std::string filename = entryPath.filename().string();

            if (entry.is_directory())
            {
                if (filename[0] == '.' || filename == "build")
                    continue;

                ImGuiTreeNodeFlags folderFlags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (g_browserFocusedPath == entryPath)
                    folderFlags |= ImGuiTreeNodeFlags_Selected;

                bool isNodeOpen = ImGui::TreeNodeEx(filename.c_str(), folderFlags);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    g_browserFocusedPath = entryPath;
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    std::string pathStr = entryPath.string();
                    ImGui::SetDragDropPayload("PROJECT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("Move folder: %s", filename.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_BROWSER_ITEM"))
                    {
                        fs::path draggedPath((const char*)payload->Data);
                        fs::path destPath = entryPath / draggedPath.filename();
                        try {
                            if (draggedPath != entryPath && draggedPath != destPath) {
                                fs::rename(draggedPath, destPath);
                                if (activeFilePath == draggedPath.string()) activeFilePath = destPath.string();
                                if (g_browserFocusedPath == draggedPath) g_browserFocusedPath = destPath;
                            }
                        }
                        catch (...) {}
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopupContextItem())
                {
                    g_browserFocusedPath = entryPath;
                    if (ImGui::MenuItem("New File..."))
                    {
                        g_popupTargetPath = entryPath;
                        g_popupNameBuffer[0] = '\0';
                        g_openNewFileModal = true;
                    }
                    if (ImGui::MenuItem("New Folder..."))
                    {
                        g_popupTargetPath = entryPath;
                        g_popupNameBuffer[0] = '\0';
                        g_openNewFolderModal = true;
                    }
                    if (ImGui::MenuItem("Rename..."))
                    {
                        g_popupTargetPath = entryPath;
                        strcpy_s(g_popupNameBuffer, filename.c_str());
                        g_openRenameModal = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete Folder", "Del"))
                    {
                        try { fs::remove_all(entryPath); if (g_browserFocusedPath == entryPath) g_browserFocusedPath = ""; }
                        catch (...) {}
                    }
                    ImGui::EndPopup();
                }

                if (isNodeOpen)
                {
                    RenderProjectTree(entryPath, editor);
                    ImGui::TreePop();
                }
            }
            else
            {
                auto extension = entryPath.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension != ".dlang" && extension != ".txt" && extension != ".md")
                    continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (activeFilePath == entryPath.string() || g_browserFocusedPath == entryPath)
                    flags |= ImGuiTreeNodeFlags_Selected;

                ImGui::TreeNodeEx(filename.c_str(), flags);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    g_browserFocusedPath = entryPath;

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    {
                        activeFilePath = entryPath.string();
                        std::ifstream t(activeFilePath);
                        if (t.is_open())
                        {
                            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                            editor.SetText(str);
                        }
                    }
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    std::string pathStr = entryPath.string();
                    ImGui::SetDragDropPayload("PROJECT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("Move file: %s", filename.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginPopupContextItem())
                {
                    g_browserFocusedPath = entryPath;
                    if (ImGui::MenuItem("Rename..."))
                    {
                        g_popupTargetPath = entryPath;
                        strcpy_s(g_popupNameBuffer, filename.c_str());
                        g_openRenameModal = true;
                    }
                    if (ImGui::MenuItem("Delete File", "Del"))
                    {
                        try {
                            if (activeFilePath == entryPath.string()) {
                                activeFilePath = "";
                                editor.SetText("");
                            }
                            fs::remove(entryPath);
                            if (g_browserFocusedPath == entryPath) g_browserFocusedPath = "";
                        }
                        catch (...) {}
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }
    catch (...) {}
}

ImFont* uiFont = nullptr;
ImFont* codeFont = nullptr;
void SetStyle(float main_scale)
{
    ImGuiIO& io = ImGui::GetIO();

    float baseFontSize = settings.settingData["editor"]["font_size"].get<float>();

    uiFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f * main_scale);
    codeFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", baseFontSize * main_scale);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;

    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ItemInnerSpacing = ImVec2(6, 6);

    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.43f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.53f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.63f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.63f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.54f, 0.73f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.44f, 0.63f, 0.90f, 1.00f);
}

int main(int, char**)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

	ideLocation = fs::current_path().string();

    settings.load();

    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DLang-IDE", nullptr };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"DLang IDE", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_HIDE);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    SetStyle(main_scale);

    TextEditor editor;
    auto lang = TextEditor::LanguageDefinition::DLang();
    editor.SetLanguageDefinition(lang);

    if (settings.settingData.contains("last_project") && settings.settingData["last_project"].is_string())
        currentProjectPath = settings.settingData["last_project"].get<std::string>();

    if (settings.settingData.contains("last_file") && settings.settingData["last_file"].is_string())
        activeFilePath = settings.settingData["last_file"].get<std::string>();

    if (!activeFilePath.empty() && fs::exists(activeFilePath))
    {
        std::ifstream t(activeFilePath);
        if (t.good())
        {
            std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
            editor.SetText(str);
        }
    }

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ScriptRunner scriptRunner;

    static auto saveFile = [&editor]()
    {
        if (!activeFilePath.empty())
        {
            auto textToSave = editor.GetText();
            std::ofstream out(activeFilePath);
            if (!out.is_open())
                return false;
            
            out << textToSave;
            out.close();
            return true;
        }
	};

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }

        if (done)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            auto cpos = editor.GetCursorPosition();

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
				if (!saveFile())
					MessageBoxA(nullptr, "Failed to save file!", "Error", MB_ICONERROR);

            ImGui::PushFont(uiFont);

            ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(0, 0, 0, 0));

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("DLang IDE", nullptr, windowFlags);

            int loadedWidth = settings.settingData["window"]["width"].get<int>();
            int loadedHeight = settings.settingData["window"]["height"].get<int>();
            ImGui::SetWindowSize(ImVec2((float)loadedWidth, (float)loadedHeight), ImGuiCond_FirstUseEver);

            if (ImGui::BeginMenuBar())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled(" DLang IDE ");
                ImGui::SameLine();
                ImGui::PopStyleVar();

                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Open Folder..."))
                    {
                        std::string selectedFolder = ChooseFolderDialog();
                        if (!selectedFolder.empty())
                        {
                            currentProjectPath = selectedFolder;
                        }
                    }
                    ImGui::Separator();

                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        if(!saveFile())
							MessageBoxA(nullptr, "Failed to save file!", "Error", MB_ICONERROR);
                    }
                    if (ImGui::MenuItem("Quit", "Alt+F4"))
                        done = true;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    bool ro = editor.IsReadOnly();
                    if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
                        editor.SetReadOnly(ro);
                    ImGui::Separator();

                    if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                        editor.Undo();
                    if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                        editor.Redo();

                    ImGui::Separator();

                    if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                        editor.Copy();
                    if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                        editor.Cut();
                    if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                        editor.Delete();
                    if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                        editor.Paste();

                    ImGui::Separator();

                    if (ImGui::MenuItem("Select all", nullptr, nullptr))
                        editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    if (ImGui::MenuItem("Dark palette"))
                        editor.SetPalette(TextEditor::GetDarkPalette());
                    if (ImGui::MenuItem("Light palette"))
                        editor.SetPalette(TextEditor::GetLightPalette());
                    if (ImGui::MenuItem("Retro blue palette"))
                        editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Compile"))
                {
                    if (ImGui::MenuItem("Build", "Ctrl+B"))
                    {
                        consoleOutput = "[PIPELINE] Starting synchronized pipeline...\n";
                        scriptRunner.SetRunning(true);

                        scriptRunner.SetRunning(true);

                        std::string activeScript = activeFilePath;

                        std::thread([activeScript, &scriptRunner]() {
                            CMakeBuilder builder;
                            std::string logOutput = "";

                            fs::path exePath = fs::current_path();
                            fs::path foundIdePath = "";

                            for (int i = 0; i < 4; ++i) {
                                if (fs::exists(exePath / "compiler_core")) {
                                    foundIdePath = exePath;
                                    break;
                                }
                                if (exePath.has_parent_path()) {
                                    exePath = exePath.parent_path();
                                }
                            }

                            if (foundIdePath.empty()) {
                                foundIdePath = fs::current_path();
                            }

                            std::string idePathStr = foundIdePath.string();
                            printf("[Pipeline] Found IDE Root at: %s\n", idePathStr.c_str());

                            if (builder.GenerateBytecodeHeader(idePathStr, activeScript, logOutput))
                                if (builder.VerifyProjectStructure(idePathStr, logOutput))
                                    builder.CompileProject(idePathStr, activeFilePath, logOutput);

                            consoleOutput += logOutput;
                            scriptRunner.SetRunning(false);
                    }).detach();

                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("Update Compiler Core"))
                    {
                        if (scriptRunner.IsRunning())
                        {
							consoleOutput += "[WARNING] Cannot update compiler core while a script is running.\n";
                            MessageBoxA(nullptr, "You cannot update while script is running!", "Script Running", MB_ICONINFORMATION);
                        }
                        else
                        {
                            consoleOutput += "Updating compiler core...\n";
                            scriptRunner.SetRunning(true);

                            std::string projectPath = currentProjectPath;

                            std::thread([projectPath, &scriptRunner]() {
                                CMakeDownloader downloader;
                                std::string logOutput = "";

                                bool success = downloader.DownloadBuildBridge(ideLocation, logOutput);
                                consoleOutput += logOutput;

                                if (!success)
                                {
                                    MessageBoxA(nullptr, "Failed to update compiler core! Check the console output for more details.", "Update Failed", MB_ICONERROR);
                                }
                                else
                                {
                                    consoleOutput += "[SUCCESS] Compiler core update finished.\n";
									MessageBoxA(nullptr, "Update completed successfully! You can now build your projects with the new compiler core.", "Update Successful", MB_ICONINFORMATION);
                                }

                                scriptRunner.SetRunning(false);
                            }).detach();
                        }
                    }

                    ImGui::EndMenu();
                }

                float menuBarWidth = ImGui::GetWindowContentRegionMax().x;
                float buttonsWidth = 150.0f;
                ImGui::SameLine(menuBarWidth - buttonsWidth);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));

                if (scriptRunner.IsRunning())
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Running...");
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));

                    if (ImGui::Button("EXEC"))
                        scriptRunner.Start(activeFilePath);

                    ImGui::PopStyleColor();
                }

                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("X"))
                {
                    done = true;
                }
                ImGui::PopStyleColor();

                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                ImGui::EndMenuBar();
            }

            ImGui::BeginChild("ProjectBrowser", ImVec2(220, -ImGui::GetFrameHeightWithSpacing()), true);
            ImGui::Text("PROJECT BROWSER");
            ImGui::Separator();

            if (!currentProjectPath.empty() && fs::exists(currentProjectPath))
            {
                RenderProjectTree(currentProjectPath, editor);

                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                {
                    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
                    {
                        if (!g_browserFocusedPath.empty() && fs::is_directory(g_browserFocusedPath))
                            g_popupTargetPath = g_browserFocusedPath;
                        else
                            g_popupTargetPath = currentProjectPath;

                        g_popupNameBuffer[0] = '\0';
                        g_openNewFileModal = true;
                    }

                    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !g_browserFocusedPath.empty())
                    {
                        try {
                            if (fs::is_directory(g_browserFocusedPath)) {
                                fs::remove_all(g_browserFocusedPath);
                            }
                            else {
                                if (activeFilePath == g_browserFocusedPath.string()) {
                                    activeFilePath = "";
                                    editor.SetText("");
                                }
                                fs::remove(g_browserFocusedPath);
                            }
                            g_browserFocusedPath = "";
                        }
                        catch (...) {}
                    }
                }

                float remainingHeight = ImGui::GetContentRegionAvail().y;
                if (remainingHeight < 40.0f) remainingHeight = 40.0f;

                ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, remainingHeight));

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_BROWSER_ITEM"))
                    {
                        fs::path draggedPath((const char*)payload->Data);
                        fs::path destPath = fs::path(currentProjectPath) / draggedPath.filename();
                        try {
                            if (draggedPath.parent_path() != currentProjectPath) {
                                fs::rename(draggedPath, destPath);
                                if (activeFilePath == draggedPath.string()) activeFilePath = destPath.string();
                                if (g_browserFocusedPath == draggedPath) g_browserFocusedPath = destPath;
                            }
                        }
                        catch (...) {}
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::BeginPopupContextWindow("ProjectBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
                {
                    if (ImGui::BeginMenu("New..."))
                    {
                        if (ImGui::MenuItem("File in Root", "Ctrl+N"))
                        {
                            g_popupTargetPath = currentProjectPath;
                            g_popupNameBuffer[0] = '\0';
                            g_openNewFileModal = true;
                        }
                        if (ImGui::MenuItem("Folder in Root"))
                        {
                            g_popupTargetPath = currentProjectPath;
                            g_popupNameBuffer[0] = '\0';
                            g_openNewFolderModal = true;
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndPopup();
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No project open.\nUse File -> Open Folder");
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginGroup();

            std::string displayFilename = activeFilePath.empty() ? "UNTITLED" : fs::path(activeFilePath).filename().string();

            ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
                editor.IsOverwrite() ? "Ovr" : "Ins",
                editor.CanUndo() ? "*" : " ",
                editor.GetLanguageDefinition().mName.c_str(), displayFilename.c_str());

            float availableHeight = ImGui::GetContentRegionAvail().y;
            float consoleHeight = 180.0f;
            float editorHeight = availableHeight - consoleHeight - ImGui::GetStyle().ItemSpacing.y;

            ImGui::BeginChild("EditorArea", ImVec2(0, editorHeight), false);
            ImGui::PushFont(codeFont);
            editor.Render("TextEditor");
            ImGui::PopFont();
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextDisabled("TERMINAL OUTPUT");

            std::string scriptOutput = scriptRunner.GetOutput();

            ImGui::BeginChild("ConsoleArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::PushFont(codeFont);

            if (!scriptOutput.empty())
                ImGui::TextUnformatted(scriptOutput.c_str());
            else
                ImGui::TextUnformatted(consoleOutput.c_str());

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
                ImGui::SetScrollHereY(1.0f);

            ImGui::PopFont();
            ImGui::EndChild();

            ImGui::EndGroup();

            settings.settingData["window"]["width"] = (int)ImGui::GetWindowWidth();
            settings.settingData["window"]["height"] = (int)ImGui::GetWindowHeight();

            if (g_openNewFileModal) { ImGui::OpenPopup("Create File"); g_openNewFileModal = false; }
            if (g_openNewFolderModal) { ImGui::OpenPopup("Create Folder"); g_openNewFolderModal = false; }
            if (g_openRenameModal) { ImGui::OpenPopup("Rename Item"); g_openRenameModal = false; }

            if (ImGui::BeginPopupModal("Create File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter new file name:");
                ImGui::InputText("##newfilename", g_popupNameBuffer, IM_ARRAYSIZE(g_popupNameBuffer));

                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    if (strlen(g_popupNameBuffer) > 0) {
                        fs::path newFilePath = g_popupTargetPath / g_popupNameBuffer;
                        std::ofstream file(newFilePath);
                        file.close();
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter new folder name:");
                ImGui::InputText("##newfoldername", g_popupNameBuffer, IM_ARRAYSIZE(g_popupNameBuffer));

                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    if (strlen(g_popupNameBuffer) > 0) {
                        fs::path newFolderPath = g_popupTargetPath / g_popupNameBuffer;
                        fs::create_directories(newFolderPath);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Rename Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Edit name:");
                ImGui::InputText("##renamebuffer", g_popupNameBuffer, IM_ARRAYSIZE(g_popupNameBuffer));

                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
                {
                    if (strlen(g_popupNameBuffer) > 0) {
                        try {
                            fs::path renamedPath = g_popupTargetPath.parent_path() / g_popupNameBuffer;
                            fs::rename(g_popupTargetPath, renamedPath);

                            // Zorg dat de editor niet breekt als het actieve bestand hernoemd is
                            if (activeFilePath == g_popupTargetPath.string()) {
                                activeFilePath = renamedPath.string();
                            }
                        }
                        catch (...) {}
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            ImGui::End();

            ImGui::PopStyleColor(3);
            ImGui::PopFont();
        }

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    settings.settingData["last_project"] = currentProjectPath;
    settings.settingData["last_file"] = activeFilePath;
    settings.save();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(g_pSwapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
    {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}