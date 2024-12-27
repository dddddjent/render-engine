#pragma once

#include <filesystem>
#include <vector>

std::vector<char> readFile(const std::filesystem::path& path);
