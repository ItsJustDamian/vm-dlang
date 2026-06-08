#pragma once
#include <iostream>
#include "imgui/imgui.h"
#include "globals.hpp"
#include "addons/cmakebuilder.hpp"
#include "addons/cmakedownloader.hpp"

namespace ui 
{

    inline fs::path g_browserFocusedPath = "";
    inline fs::path g_popupTargetPath = "";
    inline char g_popupNameBuffer[128] = "";
    inline bool g_openNewFileModal = false;
    inline bool g_openNewFolderModal = false;
    inline bool g_openRenameModal = false;

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
                                    if (g::activeFilePath == draggedPath.string()) g::activeFilePath = destPath.string();
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

                    if (extension != ".dlang" && extension != ".txt" && extension != ".md" && extension != ".json")
                        continue;

                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if (g::activeFilePath == entryPath.string() || g_browserFocusedPath == entryPath)
                        flags |= ImGuiTreeNodeFlags_Selected;

                    ImGui::TreeNodeEx(filename.c_str(), flags);

                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
                    {
                        g_browserFocusedPath = entryPath;

                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                        {
                            g::activeFilePath = entryPath.string();
                            std::ifstream t(g::activeFilePath);
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
                        if (ImGui::MenuItem("Make Primary File"))
                        {
                            projectSettings.load();
							projectSettings.projectData["primary_file"] = entryPath.filename().string();
							projectSettings.save();
                        }
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
                                if (g::activeFilePath == entryPath.string()) {
                                    g::activeFilePath = "";
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

    void DrawMenuBar(bool* done)
    {
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
                        g::currentProjectPath = selectedFolder;
                    }
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    if (!helpers::SaveFile())
                        MessageBoxA(nullptr, "Failed to save file!", "Error", MB_ICONERROR);
                }
                if (ImGui::MenuItem("Quit", "Alt+F4"))
                    *done = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                bool ro = g::editor.IsReadOnly();
                if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
                    g::editor.SetReadOnly(ro);
                ImGui::Separator();

                if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && g::editor.CanUndo()))
                    g::editor.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && g::editor.CanRedo()))
                    g::editor.Redo();

                ImGui::Separator();

                if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, g::editor.HasSelection()))
                    g::editor.Copy();
                if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && g::editor.HasSelection()))
                    g::editor.Cut();
                if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && g::editor.HasSelection()))
                    g::editor.Delete();
                if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                    g::editor.Paste();

                ImGui::Separator();

                if (ImGui::MenuItem("Select all", nullptr, nullptr))
                    g::editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(g::editor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Dark palette"))
                    g::editor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette"))
                    g::editor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette"))
                    g::editor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Compile"))
            {
                if (ImGui::MenuItem("Build", "Ctrl+B"))
                {
                    g::consoleOutput = "[PIPELINE] Starting synchronized pipeline...\n";
                    g::scriptRunner.SetRunning(true);

                    g::scriptRunner.SetRunning(true);

                    std::string activeScript = g::activeFilePath;

                    std::thread([activeScript]() {
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
                                builder.CompileProject(idePathStr, g::activeFilePath, logOutput);

                        g::consoleOutput += logOutput;
                        g::scriptRunner.SetRunning(false);
                        }).detach();

                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Update Compiler Core"))
                {
                    if (g::scriptRunner.IsRunning())
                    {
                        g::consoleOutput += "[WARNING] Cannot update compiler core while a script is running.\n";
                        MessageBoxA(nullptr, "You cannot update while script is running!", "Script Running", MB_ICONINFORMATION);
                    }
                    else
                    {
                        g::consoleOutput += "Updating compiler core...\n";
                        g::scriptRunner.SetRunning(true);

                        std::string projectPath = g::currentProjectPath;

                        std::thread([projectPath]() {
                            CMakeDownloader downloader;
                            std::string logOutput = "";

                            bool success = downloader.DownloadBuildBridge(g::ideLocation, logOutput);
                            g::consoleOutput += logOutput;

                            if (!success)
                            {
                                MessageBoxA(nullptr, "Failed to update compiler core! Check the console output for more details.", "Update Failed", MB_ICONERROR);
                            }
                            else
                            {
                                g::consoleOutput += "[SUCCESS] Compiler core update finished.\n";
                                MessageBoxA(nullptr, "Update completed successfully! You can now build your projects with the new compiler core.", "Update Successful", MB_ICONINFORMATION);
                            }

                            g::scriptRunner.SetRunning(false);
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

            if (g::scriptRunner.IsRunning())
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Running...");
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));

                if (ImGui::Button("EXEC"))
                    g::scriptRunner.Start(GetProjectPrimaryFile());

                ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("X"))
                *done = true;

            ImGui::PopStyleColor();

            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::EndMenuBar();
        }
    }

    void DrawModals()
    {
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
                        if (g::activeFilePath == g_popupTargetPath.string()) {
                            g::activeFilePath = renamedPath.string();
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
    }

	void DrawEditor(bool* done)
	{
		static auto io = ImGui::GetIO();
        auto cpos = g::editor.GetCursorPosition();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
            if (!helpers::SaveFile())
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

		DrawMenuBar(done);

        ImGui::BeginChild("ProjectBrowser", ImVec2(220, -ImGui::GetFrameHeightWithSpacing()), true);
        ImGui::Text("PROJECT BROWSER");
        ImGui::Separator();

        if (!g::currentProjectPath.empty() && fs::exists(g::currentProjectPath))
        {
            RenderProjectTree(g::currentProjectPath, g::editor);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            {
                if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N))
                {
                    if (!g_browserFocusedPath.empty() && fs::is_directory(g_browserFocusedPath))
                        g_popupTargetPath = g_browserFocusedPath;
                    else
                        g_popupTargetPath = g::currentProjectPath;

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
                            if (g::activeFilePath == g_browserFocusedPath.string()) {
                                g::activeFilePath = "";
                                g::editor.SetText("");
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
                    fs::path destPath = fs::path(g::currentProjectPath) / draggedPath.filename();
                    try {
                        if (draggedPath.parent_path() != g::currentProjectPath) {
                            fs::rename(draggedPath, destPath);
                            if (g::activeFilePath == draggedPath.string()) g::activeFilePath = destPath.string();
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
                        g_popupTargetPath = g::currentProjectPath;
                        g_popupNameBuffer[0] = '\0';
                        g_openNewFileModal = true;
                    }
                    if (ImGui::MenuItem("Folder in Root"))
                    {
                        g_popupTargetPath = g::currentProjectPath;
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

        std::string displayFilename = g::activeFilePath.empty() ? "UNTITLED" : fs::path(g::activeFilePath).filename().string();

        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, g::editor.GetTotalLines(),
            g::editor.IsOverwrite() ? "Ovr" : "Ins",
            g::editor.CanUndo() ? "*" : " ",
            g::editor.GetLanguageDefinition().mName.c_str(), displayFilename.c_str());

        float availableHeight = ImGui::GetContentRegionAvail().y;
        float consoleHeight = 180.0f;
        float editorHeight = availableHeight - consoleHeight - ImGui::GetStyle().ItemSpacing.y;

        ImGui::BeginChild("EditorArea", ImVec2(0, editorHeight), false);
        ImGui::PushFont(codeFont);
        g::editor.Render("TextEditor");
        ImGui::PopFont();
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::TextDisabled("TERMINAL OUTPUT");

        std::string scriptOutput = g::scriptRunner.GetOutput();

        ImGui::BeginChild("ConsoleArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(codeFont);

        if (!scriptOutput.empty())
            ImGui::TextUnformatted(scriptOutput.c_str());
        else
            ImGui::TextUnformatted(g::consoleOutput.c_str());

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::PopFont();
        ImGui::EndChild();

        ImGui::EndGroup();

        settings.settingData["window"]["width"] = (int)ImGui::GetWindowWidth();
        settings.settingData["window"]["height"] = (int)ImGui::GetWindowHeight();

        DrawModals();

        ImGui::End();

        ImGui::PopStyleColor(3);
        ImGui::PopFont();
	}

	void render(bool* done)
	{
		DrawEditor(done);
	}
}