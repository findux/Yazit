#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>

std::string OpenFileDialog(HWND owner = nullptr);
std::string SaveFileDialog(const std::string& current = {}, HWND owner = nullptr);
