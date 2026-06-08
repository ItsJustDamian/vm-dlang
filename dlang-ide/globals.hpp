#pragma once
#include <iostream>
#include "addons/helpers.hpp"
#include "addons/scriptrunner.hpp"
#include "imgui-addons/TextEditor.h"

namespace g
{
	ScriptRunner scriptRunner;
	TextEditor editor;

	inline std::string currentProjectPath = "";
	inline std::string activeFilePath = "";
	inline std::string consoleOutput = "";
	inline std::string ideLocation = "";
	inline bool g_isWindowMaximized = false;
}

class ProjectSettings
{
public:
    nlohmann::json projectData;

    bool load()
    {
        std::ifstream i(g::currentProjectPath + "/project.json");
        if (i.is_open())
        {
            i >> projectData;
            i.close();
            return true;
        }

        return false;
	}

    bool save()
    {
		std::ofstream o(g::currentProjectPath + "/project.json");
		if (o.is_open())
        {
            o << projectData.dump(4);
            o.close();
            return true;
        }

        return false;
    }
} inline projectSettings;

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

inline std::string GetProjectPrimaryFile()
{
    projectSettings.load();

    if (projectSettings.projectData.contains("primary_file"))
        return projectSettings.projectData["primary_file"].get<std::string>();

    if (!g::activeFilePath.empty())
        return std::filesystem::path(g::activeFilePath).filename().string();

    return "untitled.dlang";
}