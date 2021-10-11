#pragma once

#include <filesystem>

namespace usagi
{
class AppDirectory
{
protected:
    std::filesystem::path mBaseFolder;

    static constexpr auto SUBFOLDER_DB_WORLD = "world";
    static constexpr auto SUBFOLDER_DB_INPUT = "input";
    static constexpr auto SUBFOLDER_NAMED_HEAPS = "heaps";

    void init(std::filesystem::path base_folder)
    {
        mBaseFolder = std::move(base_folder);

        create_directories(mBaseFolder);
        create_directories(mBaseFolder / SUBFOLDER_DB_WORLD);
        create_directories(mBaseFolder / SUBFOLDER_DB_INPUT);
        create_directories(mBaseFolder / SUBFOLDER_NAMED_HEAPS);
    }
};
}
