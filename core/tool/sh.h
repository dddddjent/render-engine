#pragma once

#include <filesystem>
#include <string>

std::string exec(const std::string& cmd);

std::string sed(const std::string& expr, const std::filesystem::path& in, const std::filesystem::path& out);

std::string replaceDefine(
    const std::string& name,
    const int value,
    const std::filesystem::path& in,
    const std::filesystem::path& out);
std::string replaceDefine(
    const std::string& name,
    const float value,
    const std::filesystem::path& in,
    const std::filesystem::path& out);
std::string replaceInclude(
    const std::string& prev_include,
    const std::string& new_include,
    const std::filesystem::path& in, const std::filesystem::path& out);

std::string glslc(const std::filesystem::path& in, const std::filesystem::path& out);
