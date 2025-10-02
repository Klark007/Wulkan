#pragma once

#include "fmt/core.h"
#include <fmt/ranges.h>

#include "Exception.h"

#include <filesystem>
#include <vector>

typedef std::filesystem::path VKW_Path;

template<>
struct fmt::formatter<VKW_Path> : fmt::formatter<std::string>
{
    auto format(VKW_Path p, format_context& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", p.string());
    }
};

inline VKW_Path find_first_existing(const std::vector<VKW_Path>& paths)
{
    for (const VKW_Path& p : paths) {
        if (std::filesystem::exists(p)) {
            return p;
        }
    }

    throw IOException(
        fmt::format("Failed to find any file from list of possibilites: {}", paths),
        __FILE__, __LINE__
    );
}