#pragma once

#include <conio.h>
#include <string>
#include <vector>
#include <windows.h>
#include <iostream>

int SelectOption(const std::vector<std::string>& options)
{
    int selected = 0;

    while (true)
    {
        // Move cursor to the beginning of the menu
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD pos = { 0, 0 };
        SetConsoleCursorPosition(console, pos);

        for (int i = 0; i < options.size(); ++i)
        {
            if (i == selected)
                std::cout << "> " << options[i] << "   \n";
            else
                std::cout << "  " << options[i] << "   \n";
        }

        int key = _getch();

        if (key == 0 || key == 224)
        {
            // Arrow keys return two codes
            key = _getch();

            if (key == 72)       // Up arrow
                selected = (selected - 1 + options.size())
                % options.size();
            else if (key == 80)  // Down arrow
                selected = (selected + 1) % options.size();
        }
        else if (key == 13)      // Enter
        {
            return selected;
        }
        else if (key == 27)      // Escape
        {
            return -1;
        }
    }
}

std::vector<char*> ScanProblems() {
	
}