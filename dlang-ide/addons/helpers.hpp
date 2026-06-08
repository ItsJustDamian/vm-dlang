#pragma once
#include <iostream>
#include "../globals.hpp"

namespace helpers
{
    bool SaveFile()
    {
        if (!g::activeFilePath.empty())
        {
            auto textToSave = g::editor.GetText();
            std::ofstream out(g::activeFilePath);
            if (!out.is_open())
                return false;

            out << textToSave;
            out.close();
            return true;
        }
    };
}



