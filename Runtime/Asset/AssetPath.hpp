#pragma once

#include <ranges>

#include <Usagi/Library/Utilities/MaybeOwnedString.hpp>

namespace usagi
{
class AssetPath : MaybeOwnedString
{
    mutable std::optional<std::string> mReconstructed;

public:
    using MaybeOwnedString::MaybeOwnedString;

    auto normalized_components() const
    {
        constexpr std::string_view delimiter("/");
        return view() |
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
    //         usagi::AssetPath pp (p);
    //         std::cout << pp.reconstructed() << std::endl;
    //     }
    // }
    //
    // output:
    //
    // /C:/text/a.txt/.././..
    // abc/text/a.txt/.././..
    //
    //
    //
    // 123
    std::string reconstructed() const;

    // todo remove
    friend void append_bytes(auto append_func, const AssetPath &p)
    {
        append_func(p.reconstructed());
    }

    friend std::string_view to_string_view(const AssetPath &p)
    {
        if(!p.mReconstructed)
            p.mReconstructed = p.reconstructed();
        return p.mReconstructed.value();
    }
};
}
