#include "AssetPath.hpp"

// std::ranges::copy
#include <algorithm>

namespace usagi
{
std::string AssetPath::reconstructed() const
{
    std::string path;
    bool last_empty = false;
    for(auto &&c : normalized_components())
    {
        // merge consecutive slashes
        if(((last_empty = std::ranges::empty(c))) && !path.empty())
            continue;
        std::ranges::copy(c, std::back_inserter(path));
        path += "/";
    }
    // if the original path was not ended with a slash, remove it.
    if(!last_empty && !path.empty()) path.pop_back();
    return path;
}
}
