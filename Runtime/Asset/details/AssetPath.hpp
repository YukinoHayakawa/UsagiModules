#pragma once

#include <string_view>
#include <ranges>

namespace usagi
{
class AssetPath
{
    std::string_view mPath;

public:
    AssetPath(const char *path)
        : mPath(path)
    {
    }

    AssetPath(std::string_view path)
        : mPath(path)
    {
    }

    auto normalized_components() const
    {
        constexpr std::string_view delimiter("/");
        return mPath |
            // normalize to slashes
            std::views::transform([](char c) {
                if(c == '\\') return '/'; return c;
            }) |
            // split path in to components by slashes. note that two adjacent
            // slashes will generate an empty subview.
            std::views::split(delimiter);
    }

    // test:
    //
    // #include <iostream>
    //
    // int main()
    // {
    //     auto paths = {
    //         "/C:\\\\text/a.txt/.././///../",
    //         "abc/\\/\\text///a.txt/.././///..",
    //         "",
    //         "\\\\",
    //         "/",
    //         "123",
    //     };
    //     for(auto &&p : paths)
    //     {
    //         AssetPath pp (p);
    //         std::cout << pp.reconstructed() << std::endl;
    //     }
    // }
    //
    // output:
    //
    // /C:/text/a.txt/.././../
    // abc/text/a.txt/.././..
    //
    // /
    // /
    // 123
    std::string reconstructed() const;

    friend void append_bytes(auto append_func, const AssetPath &p)
    {
        append_func(p.reconstructed());
    }
};
}
